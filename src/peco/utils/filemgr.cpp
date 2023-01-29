/*
    filemgr.cpp
    libpeco
    2019-10-30
    Push Chen
*/

/*
MIT License

Copyright (c) 2019 Push Chen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "peco/utils/filemgr.h"
#include "peco/utils/stringutil.h"

#if !PECO_TARGET_WIN
#include <dirent.h>
#else
#include <direct.h>
#include <windows.h>
#endif
#include <stack>

#if PECO_TARGET_WIN
#define PECO_F_STAT _stat
#define PECO_F_IFDIR _S_IFDIR
#define PECO_F_MKDIR(d) _mkdir(d)
#define PECO_F_REMOVE remove
#define PECO_F_RMDIR _rmdir
#else

#include <cstdio>
#include <fcntl.h>

#ifndef ACCESSPERMS
#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)
#endif

#define PECO_F_STAT stat
#define PECO_F_IFDIR S_IFDIR
#define PECO_F_MKDIR(d) mkdir(d, ACCESSPERMS)
#define PECO_F_REMOVE remove
#define PECO_F_RMDIR rmdir
#endif

#include <algorithm>

namespace peco {

// Check if folder existed
bool is_folder_existed(const std::string &path) {
  struct PECO_F_STAT _dir_info;
  if (PECO_F_STAT(path.c_str(), &_dir_info) != 0)
    return false;
  return (bool)(_dir_info.st_mode & S_IFDIR);
}

// Check if file existed
bool is_file_existed(const std::string &path) {
  struct PECO_F_STAT _f_info;
  if (PECO_F_STAT(path.c_str(), &_f_info) != 0)
    return false;
  return !(bool)(_f_info.st_mode & PECO_F_IFDIR);
}

// Create a folder at the path
bool make_dir(const std::string &path) {
  if (PECO_F_MKDIR(path.c_str()) == 0)
    return true;
  // If already existed, also success
  return errno == EEXIST;
}

// Create multiple levels of directories
bool rek_make_dir(const std::string &path) {
  auto _path = peco::split(path, "/");
  std::string _dpath;
  if (path[0] == '/')
    _dpath = "/";
  while (_path.size() > 0) {
    _dpath += (_path[0] + "/");
    if (!make_dir(_dpath))
      return false;
    _path.erase(_path.begin());
  }
  return true;
}

// Get dirname of a path
std::string dir_name(const std::string &path) {
#if PECO_TARGET_WIN
  size_t _l1 = path.rfind('\\');
  size_t _l2 = path.find('/');
  size_t _l = (_l1 == std::string::npos
                 ? _l2
                 : (_l2 == std::string::npos ? _l1 : std::max(_l1, _l2)));
  if (_l == std::string::npos)
    return path;
  return path.substr(0, _l + 1);
#else
  size_t _l = path.rfind('/');
  if (_l == std::string::npos)
    return "./";
  return path.substr(0, _l + 1); // Include '/'
#endif
}

// Get the filename of a path
std::string file_name(const std::string &path) {
  std::string _ff(path);
  size_t _l = _ff.rfind('.');
  if (_l == std::string::npos)
    return _ff;
  return _ff.substr(0, _l);
}

// Get the full filename of a path
std::string full_file_name(const std::string &path) {
#if PECO_TARGET_WIN
  size_t _l1 = path.rfind('\\');
  size_t _l2 = path.find('/');
  size_t _l = (_l1 == std::string::npos
                 ? _l2
                 : (_l2 == std::string::npos ? _l1 : std::max(_l1, _l2)));
  if (_l == std::string::npos)
    return path;
  return path.substr(_l + 1);
#else
  size_t _l = path.rfind('/');
  if (_l == std::string::npos)
    return path;
  return path.substr(_l + 1);
#endif
}

// Get the ext of a path
std::string extension(const std::string &path) {
  std::string _fpath = full_file_name(path);
  size_t _l = _fpath.rfind('.');
  if (_l == std::string::npos)
    return std::string("");
  // Ignore hidden file's dot
  if (_l == 0)
    return std::string("");
  return _fpath.substr(_l + 1);
}

// Get update time of a given file
time_t file_update_time(const std::string &path) {
  struct PECO_F_STAT _finfo;
  if (PECO_F_STAT(path.c_str(), &_finfo) != 0)
    return (time_t)0;
#ifdef __APPLE__
  return _finfo.st_mtimespec.tv_sec;
#else
  return _finfo.st_mtime;
#endif
}

#if PECO_TARGET_WIN

#define DT_UNKNOWN 0
#define DT_FILE 1
#define DT_DIR 2

struct dirent {
  unsigned short d_reclen; /* length of this d_name*/
  unsigned char d_type;    /* the type of d_name*/
  char d_name[1];          /* file name (null-terminated)*/
};

typedef struct _dirdesc {
  HANDLE dd_find;
  struct dirent dd_cur;
} DIR;

#define W_DIR_NAME_BUF_SIZE 512

DIR *opendir(const char *name) {
  DIR *_dir = (DIR *)malloc(sizeof(DIR));
  if (!_dir) {
    printf("DIR memory allocate fail\n");
    return nullptr;
  }
  memset(_dir, 0, sizeof(DIR));

  WIN32_FIND_DATA _find_data;
  char namebuf[W_DIR_NAME_BUF_SIZE];
  sprintf_s(namebuf, W_DIR_NAME_BUF_SIZE, "%s\\*.*", name);

  _dir->dd_find = FindFirstFile(namebuf, &_find_data);
  if (_dir->dd_find == INVALID_HANDLE_VALUE) {
    printf("FindFirstFile failed (%d)\n", GetLastError());
    return nullptr;
  }

  return _dir;
}

struct dirent *readdir(DIR *d) {
  if (!d) {
    return nullptr;
  }

  WIN32_FIND_DATA _file_data;
  BOOL bf = FindNextFile(d->dd_find, &_file_data);
  // fail or end
  if (!bf) {
    return nullptr;
  }

  int i;
  for (i = 0; i < 256 && _file_data.cFileName[i] != '\0'; i++) {
    d->dd_cur.d_name[i] = _file_data.cFileName[i];
  }
  d->dd_cur.d_name[i] = '\0';
  d->dd_cur.d_reclen = i;

  // check there is file or directory
  if (_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    d->dd_cur.d_type = DT_DIR;
  } else {
    d->dd_cur.d_type = DT_FILE;
  }
  return (&d->dd_cur);
}

int closedir(DIR *d) {
  if (!d)
    return -1;
  free(d);
  return 0;
}

#endif

// Scan a given dir and get all files.
void scan_dir(const std::string &path, dir_filter_t filter,
              bool ignore_hidden) {
  std::string _currentpath = path;
  if (*_currentpath.rbegin() != '/')
    _currentpath += '/';

  DIR *_dir = opendir(path.c_str());
  if (_dir == NULL)
    return;

  struct dirent *_node;
  while ((_node = readdir(_dir)) != NULL) {
    std::string _dpath(_node->d_name);
    if (_dpath == "." || _dpath == "..")
      continue;
    // Check if ignore hidden file/folders
    if (_dpath[0] == '.' && ignore_hidden)
      continue;
    if (!filter)
      continue; // do nothing
    filter(_currentpath + _dpath, (_node->d_type == DT_DIR));
  }
  closedir(_dir);
}

// Scan all levels
void rek_scan_dir(const std::string &path, dir_filter_t filter,
                  bool ignore_hidden) {
  std::string _currentpath = path;
  if (*_currentpath.rbegin() != '/')
    _currentpath += '/';

  DIR *_dir = opendir(path.c_str());
  if (_dir == NULL)
    return;

  struct dirent *_node;
  while ((_node = readdir(_dir)) != NULL) {
    // Ignore unknow node
    if (_node->d_type == DT_UNKNOWN)
      continue;

    std::string _dpath(_node->d_name);
    if (_dpath == "." || _dpath == "..")
      continue;
    // Check if ignore hidden file/folders
    if (_dpath[0] == '.' && ignore_hidden)
      continue;
    if (!filter)
      continue; // do nothing
    if (filter(_currentpath + _dpath, (_node->d_type == DT_DIR))) {
      // go on scan if the filter return true
      if (_node->d_type != DT_DIR)
        continue; // skip file
      rek_scan_dir(_currentpath + _dpath, filter, ignore_hidden);
    }
  }
  closedir(_dir);
}
// Remove a folder or file
void fs_remove(const std::string &path) {
  if (is_file_existed(path)) {
    // This is file
    PECO_F_REMOVE(path.c_str());
  }
  if (is_folder_existed(path)) {
    // This is folder
    std::stack<std::pair<std::string, bool>> _cache;
    rek_scan_dir(
      path,
      [&_cache](const std::string &p, bool f) {
        _cache.emplace(std::make_pair(p, f));
        return true;
      },
      false);
    while (_cache.size() != 0) {
      auto &_p = _cache.top();
      if (_p.second == true) {
        PECO_F_RMDIR(_p.first.c_str());
      } else {
        PECO_F_REMOVE(_p.first.c_str());
      }
      _cache.pop();
    }
  }
  // do nothing if not existed
}
// Get file size
size_t fs_size(const std::string& path) {
  struct PECO_F_STAT _f_info;
  if (PECO_F_STAT(path.c_str(), &_f_info) != 0)
    return 0;
  if ((bool)(_f_info.st_mode & PECO_F_IFDIR)) {
    // this is folder
    return 0;
  }
  return _f_info.st_size;
}

} // namespace peco

// Push Chen
