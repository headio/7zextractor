#include "stdafx.h"
#include "7zFileStream.h"

HRESULT ConvertBoolToHRESULT(bool result) {
  if (result) {
    return S_OK;
  }
  DWORD lasterror = ::GetLastError();
  if (0 == lasterror) {
    return E_FAIL;
  }
  return HRESULT_FROM_WIN32(lasterror);
}

InFileStream::InFileStream(void) {

}

bool InFileStream::Open(const std::wstring& filename) {
  if (!file_.Open(filename)) {
    return false;
  }
  stream_type_ = kFile;
  return true;
}

bool InFileStream::OpenFromMemory(const void* data, UInt64 len) {
  if (data == NULL || len <= 0) {
    return false;
  }
  data_ = data;
  data_cursor_ = 0;
  data_len_ = len;
  stream_type_ = kMemory;
  return true;
}

STDMETHODIMP InFileStream::Read(void* data, UInt32 size, UInt32* processed_size) {
  if (stream_type_ == kFile) {
    DWORD real_processed;
    bool result = file_.ReadPart(data, size, &real_processed);
    if (processed_size != NULL) {
      *processed_size = real_processed;
    }
    return ConvertBoolToHRESULT(result);
  } else if (stream_type_ == kMemory) {
    if (size > data_len_ - data_cursor_) {
      return E_FAIL;
    }
    memcpy(data, (char*)data_ + data_cursor_, size);
    data_cursor_ += size;
    if (processed_size != NULL) {
      *processed_size = size;
    }
    return S_OK;
  }
  return E_ABORT;
}

STDMETHODIMP InFileStream::Seek(Int64 offset, UInt32 seek_origin, UInt64* new_position) {
  if (seek_origin > 3) {
    return STG_E_INVALIDFUNCTION;
  }
  if (stream_type_ == kFile) {
    unsigned __int64 real_new_position;
    bool result = file_.Seek(offset, &real_new_position, seek_origin);
    if (new_position != NULL) {
      *new_position = real_new_position;
    }
    return ConvertBoolToHRESULT(result);
  } else if (stream_type_ == kMemory) {
    if (seek_origin == FILE_BEGIN) {
      if ((UInt64)offset > data_len_ || offset < 0) {
        return E_FAIL;
      }
      data_cursor_ = offset;
    } else if (seek_origin == FILE_END) {
      if ((UInt64)-offset > data_len_ || offset > 0) {
        return E_FAIL;
      }
      data_cursor_ = -offset;
    } else if (seek_origin == FILE_CURRENT) {
      UInt64 pos = data_cursor_ + offset;
      if (pos > data_len_ || pos < 0) {
        return E_FAIL;
      }
      data_cursor_ = pos;
    }
    if (new_position != NULL) {
      *new_position = data_cursor_;
    }
    return S_OK;
  }
  return E_ABORT;
}

STDMETHODIMP InFileStream::GetSize(UInt64 *size) {
  if (stream_type_ == kFile) {
    *size = file_.GetSize();
    return S_OK;
  } else if (stream_type_ == kMemory) {
    *size = data_len_;
    return S_OK;
  }
  return E_ABORT;
}

bool OutFileStream::Create(const std::wstring& filename, bool create_always) {
  return file_.Create(filename, create_always);
}

STDMETHODIMP OutFileStream::Write(const void*data, UInt32 size, UInt32* processed_size) {
  DWORD real_processed;
  bool result = file_.WritePart(data, size, &real_processed);
  if (processed_size != NULL) {
    *processed_size = real_processed;
  }
  return ConvertBoolToHRESULT(result);
}

STDMETHODIMP OutFileStream::Seek(Int64 offset, UInt32 seek_origin, UInt64* new_position) {
  if (seek_origin > 3) {
    return STG_E_INVALIDFUNCTION;
  }
  unsigned __int64 real_new_position;
  bool result = file_.Seek(offset, &real_new_position, seek_origin);
  if (new_position != NULL) {
    *new_position = real_new_position;
  }
  return ConvertBoolToHRESULT(result);
}

STDMETHODIMP OutFileStream::SetSize(UInt64 new_size) {
  unsigned __int64 current_pos;
  if (!file_.Seek(0, &current_pos, FILE_CURRENT)) {
    return E_FAIL;
  }
  bool result = file_.SetSize(new_size);
  unsigned __int64 current_pos2;
  result = result && file_.Seek(current_pos, &current_pos2);
  return result ? S_OK : E_FAIL;
}

bool OutFileStream::SetMTime( const FILETIME* mtime )
{
  return file_.SetFileTime(NULL, NULL, mtime);
}

HRESULT OutFileStream::Close( void )
{
  return ConvertBoolToHRESULT(file_.Close());
}
