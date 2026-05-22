#pragma once

class TextEncoding
{
  _COM_SMARTPTR_TYPEDEF(IMultiLanguage3, __uuidof(IMultiLanguage3));

public:
  explicit TextEncoding(const void* pSrcBuffer = nullptr, size_t size = -1);
  ~TextEncoding();

public:
  void SetBuffer(const void* pSrcBuffer, size_t size);
  bool IsUnicode() const;
  int DetectCodePage();
  
  static int GetOEMCodePage();
  static int GetANSICodePage(bool bThread = false);

  size_t GetToUnicodeCharCount(int nCodePage = 0);
  bool ToUnicode(wchar_t* pDestBuffer, size_t size, int nCodePage = 0);
  
  size_t GetFromUnicodeBufferSize(int nCodePage);
  bool FromUnicode(int nCodePage, char* pDestBuffer, size_t size);

  size_t GetToStringBufferSize(int nCodePage, int nSrcCodePage = 0);
  bool ToString(int nCodePage, char* pDestBuffer, size_t size, int nSrcCodePage = 0);

protected:
  bool init();

protected:
  union BufferType
  {
    const char* String;
    const wchar_t* Unicode;
  } m_pBuffer;
  size_t m_nBufferSize;
  int m_nDefCodePage;
  _com_ptr_t<IMultiLanguage3Ptr> m_pMultiLanguage;
};
