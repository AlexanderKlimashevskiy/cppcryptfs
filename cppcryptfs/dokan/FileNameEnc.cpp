
/*
cppcryptfs : user-mode cryptographic virtual overlay filesystem.

Copyright (C) 2016-2018 Bailey Brown (github.com/bailey27/cppcryptfs)

cppcryptfs is based on the design of gocryptfs (github.com/rfjakob/gocryptfs)

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "FileNameEnc.h"
#include "libcppcryptfs/util.h"
#include "libcppcryptfs/fileutil.h"
#include "libcppcryptfs/cryptfilename.h"
#include "dokan/dokan.h"
#include "cryptdokan.h"
#include "cryptdokanpriv.h"

// Due to a bug in the Dokany driver (as of Dokany 1.03), if we set FILE_NAMED_STREAMS in
// the volume flags (in CryptGetVolumeInformation())
// to announce that we support alternate data streams in files,
// then whenever a path with a stream is sent down to us by File Explorer, there's an extra slash after the filename
// and before the colon (e.g. \foo\boo\foo.txt\:blah:$DATA).
// So here we git rid of that extra slash if necessary.

void FileNameEnc::AssignPlainPath(LPCWSTR plain_path) {

	m_plain_path = plain_path;

	// The bug mentioned above is now fixed in Dokany.  The fix should be in Dokany 1.04.
	// When Dokany 1.04 comes out, we should verify that the fix is actually there
	// and use the version to determine if we still need to do this or not.
	// But it won't hurt to leave this code in.

	LPCWSTR pColon = wcschr(plain_path, ':');

	if (!pColon)
		return;

	if (pColon == plain_path)
		return;

	if (pColon[-1] != '\\')
		return;

	m_plain_path.erase(pColon - plain_path - 1);

	m_plain_path += pColon;

	DbgPrint(L"converted file with stream path %s -> %s\n", plain_path,
		m_plain_path.c_str());
}

FileNameEnc::FileNameEnc(PDOKAN_FILE_INFO DokanFileInfo, const WCHAR *fname,
	string *actual_encrypted,
	bool forceCaseCacheNotFound) {
	m_dokan_file_info = DokanFileInfo;
	m_con = GetContext();
	AssignPlainPath(fname);
	m_actual_encrypted = actual_encrypted;
	m_tried = false;
	m_failed = false;
	m_file_existed = false;
	m_force_case_cache_notfound = forceCaseCacheNotFound;
}

FileNameEnc::~FileNameEnc() {}

const WCHAR *FileNameEnc::Convert() {

	if (!m_tried) {

		m_tried = true;

		try {
			if (m_con->GetConfig()->m_reverse) {
				if (rt_is_config_file(m_con, m_plain_path.c_str())) {
					m_enc_path = m_con->GetConfig()->m_basedir + L"\\";
					m_enc_path += REVERSE_CONFIG_NAME;
				} else if (rt_is_virtual_file(m_con, m_plain_path.c_str())) {
					wstring dirpath;
					if (!get_file_directory(m_plain_path.c_str(), dirpath))
						throw(-1);
					if (!decrypt_path(m_con, &dirpath[0], m_enc_path))
						throw(-1);
					m_enc_path += L"\\";
					wstring filename;
					if (!get_bare_filename(m_plain_path.c_str(), filename))
						throw(-1);
					m_enc_path += filename;
				} else {
					if (!decrypt_path(m_con, m_plain_path.c_str(), m_enc_path)) {
						throw(-1);
					}
				}
			} else {

				LPCWSTR plain_path = m_plain_path.c_str();
				int cache_status = CASE_CACHE_NOTUSED;
				if (m_con->IsCaseInsensitive()) {
					cache_status = m_con->m_case_cache.lookup(
						m_plain_path.c_str(), m_correct_case_path,
						m_force_case_cache_notfound);
					if (cache_status == CASE_CACHE_FOUND ||
						cache_status == CASE_CACHE_NOT_FOUND) {
						m_file_existed = cache_status == CASE_CACHE_FOUND;
						plain_path = m_correct_case_path.c_str();
					} else if (cache_status == CASE_CACHE_MISS) {
						if (m_con->m_case_cache.load_dir(m_plain_path.c_str())) {
							cache_status = m_con->m_case_cache.lookup(
								m_plain_path.c_str(), m_correct_case_path,
								m_force_case_cache_notfound);
							if (cache_status == CASE_CACHE_FOUND ||
								cache_status == CASE_CACHE_NOT_FOUND) {
								m_file_existed = cache_status == CASE_CACHE_FOUND;
								plain_path = m_correct_case_path.c_str();
							}
						}
					}
					wstring stream;
					wstring file_without_stream;
					bool have_stream =
						get_file_stream(plain_path, &file_without_stream, &stream);
					if (have_stream) {
						unordered_map<wstring, wstring> streams_map;
						wstring stream_without_type;
						wstring type;

						if (!remove_stream_type(stream.c_str(), stream_without_type,
							type)) {
							throw(-1);
						}

						if (CryptFindStreamsInternal(
							file_without_stream.c_str(), NULL, m_dokan_file_info,
							CryptCaseStreamsCallback, &streams_map) == 0) {

							wstring uc_stream;

							if (!touppercase(stream_without_type.c_str(), uc_stream))
								throw(-1);

							auto it = streams_map.find(uc_stream);

							if (it != streams_map.end()) {
								m_correct_case_path = file_without_stream + it->second + type;
								plain_path = m_correct_case_path.c_str();
								DbgPrint(L"stream found %s -> %s\n", m_plain_path, plain_path);
							} else {
								DbgPrint(L"stream not found %s -> %s\n", m_plain_path,
									plain_path);
							}
						}
					}
				}
				if (!encrypt_path(m_con, plain_path, m_enc_path, m_actual_encrypted)) {
					throw(-1);
				}
			}
		} catch (...) {
			m_failed = true;
		}
	}

	const WCHAR *rs = !m_failed ? &m_enc_path[0] : NULL;

	if (rs) {
		DbgPrint(L"\tconverted filename %s => %s\n", m_plain_path.c_str(), rs);
	} else {
		DbgPrint(L"\terror converting filename %s\n", m_plain_path.c_str());
	}

	return rs;
}
