#include "pch.h"
#include "TextEncoding.h"

TextEncoding::TextEncoding(const void* pSrcBuffer, size_t size)
{
  HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  _ASSERTE(SUCCEEDED(hr));

  SetBuffer(pSrcBuffer, size);

  m_nDefCodePage = GetANSICodePage();
}

TextEncoding::~TextEncoding()
{
  ::CoUninitialize();
}

bool TextEncoding::init()
{
  if (m_pMultiLanguage == nullptr)
  {
    HRESULT hr = m_pMultiLanguage.CreateInstance(CLSID_CMultiLanguage, nullptr, CLSCTX_INPROC_SERVER);
    _ASSERTE(hr == S_OK);
    return hr == S_OK;
  }
  return true;
}

void TextEncoding::SetBuffer(const void* pSrcBuffer, size_t size)
{
  m_pBuffer.String = reinterpret_cast<const char*>(pSrcBuffer);
  m_nBufferSize = size;
}

bool TextEncoding::IsUnicode() const
{
  return ::IsTextUnicode(m_pBuffer.String, m_nBufferSize, nullptr);
}

int TextEncoding::DetectCodePage()
{
  if (!init())
    return -1;

  int size_s = m_nBufferSize;
  DetectEncodingInfo Info = { 0 };
  int nInfoCount = 1;

  HRESULT hr = m_pMultiLanguage->DetectInputCodepage(MLDETECTCP_NONE, 0, const_cast<char*>(m_pBuffer.String), &size_s, &Info, &nInfoCount);
  if (hr == S_OK && nInfoCount > 0)
    return Info.nCodePage;

  UINT uiPreferredCodePages[2];
  uiPreferredCodePages[0] = 1200;
  uiPreferredCodePages[1] = 1201;
  UINT uiDetectedCodePages = 0;
  UINT pnDetectedCodePages = 1;

  hr = m_pMultiLanguage->DetectOutboundCodePage(MLDETECTF_VALID_NLS| MLDETECTF_FILTER_SPECIALCHAR, m_pBuffer.Unicode, m_nBufferSize / sizeof(wchar_t), 
    uiPreferredCodePages, sizeof(uiPreferredCodePages) / sizeof(UINT), &uiDetectedCodePages, &pnDetectedCodePages, nullptr);
  if (hr == S_OK && pnDetectedCodePages > 0)
    return uiDetectedCodePages;

  return m_nDefCodePage;
}

int TextEncoding::GetOEMCodePage()
{
  CPINFOEX cpInfoOEM = { 0 };
  ::GetCPInfoEx(CP_OEMCP, 0, &cpInfoOEM);
  return cpInfoOEM.CodePage;
}

int TextEncoding::GetANSICodePage(bool bThread)
{
  CPINFOEX cpInfoANSI = { 0 };
  ::GetCPInfoEx(bThread ? CP_THREAD_ACP : CP_ACP, 0, &cpInfoANSI);
  return cpInfoANSI.CodePage;
}

size_t TextEncoding::GetToUnicodeCharCount(int nCodePage)
{
  if (!init())
    return -1;

  if (!nCodePage)
    nCodePage = DetectCodePage();

  if (!nCodePage)
    return -1;

  UINT size_s = m_nBufferSize;
  UINT size_d = 0;
  DWORD dwMode = 0;

  HRESULT hr = m_pMultiLanguage->ConvertStringToUnicode(&dwMode, nCodePage, const_cast<char*>(m_pBuffer.String), &size_s, nullptr, &size_d);
  if (hr == S_OK && size_d > 0)
    return size_d;

  return -1;
}

bool TextEncoding::ToUnicode(wchar_t* pDestBuffer, size_t size, int nCodePage)
{
  if (!init())
    return false;

  if (!nCodePage)
    nCodePage = DetectCodePage();
  
  if (!nCodePage)
    return false;

  UINT size_s = m_nBufferSize;
  UINT size_d = size;
  DWORD dwMode = 0;

  HRESULT hr = m_pMultiLanguage->ConvertStringToUnicode(&dwMode, nCodePage, const_cast<char*>(m_pBuffer.String), &size_s, pDestBuffer, &size_d);
  return hr == S_OK;
}

size_t TextEncoding::GetFromUnicodeBufferSize(int nCodePage)
{
  if (!init())
    return -1;

  UINT size_s = m_nBufferSize;
  UINT size_d = 0;
  DWORD dwMode = 0;

  HRESULT hr = m_pMultiLanguage->ConvertStringFromUnicode(&dwMode, nCodePage, const_cast<wchar_t*>(m_pBuffer.Unicode), &size_s, nullptr, &size_d);
  if (hr == S_OK && size_d > 0)
    return size_d;

  return -1;
}

bool TextEncoding::FromUnicode(int nCodePage, char* pDestBuffer, size_t size)
{
  if (!init())
    return false;

  UINT size_s = m_nBufferSize;
  UINT size_d = size;
  DWORD dwMode = 0;

  HRESULT hr = m_pMultiLanguage->ConvertStringFromUnicode(&dwMode, nCodePage, const_cast<wchar_t*>(m_pBuffer.Unicode), &size_s, pDestBuffer, &size_d);
  return hr == S_OK;
}

size_t TextEncoding::GetToStringBufferSize(int nCodePage, int nSrcCodePage)
{
  if (!init())
    return -1;

  if (!nSrcCodePage)
    nSrcCodePage = DetectCodePage();

  if (!nSrcCodePage)
    return -1;

  UINT size_s = m_nBufferSize;
  UINT size_d = 0;
  DWORD dwMode = 0;

  HRESULT hr = m_pMultiLanguage->ConvertString(&dwMode, nSrcCodePage, nCodePage, reinterpret_cast<uint8_t*>(const_cast<char*>(m_pBuffer.String)), &size_s, nullptr, &size_d);
  if (hr == S_OK && size_d > 0)
    return size_d;

  return -1;
}

bool TextEncoding::ToString(int nCodePage, char* pDestBuffer, size_t size, int nSrcCodePage)
{
  if (!init())
    return false;

  if (!nSrcCodePage)
    nSrcCodePage = DetectCodePage();

  if (!nSrcCodePage)
    return false;

  UINT size_s = m_nBufferSize;
  UINT size_d = 0;
  DWORD dwMode = 0;

  HRESULT hr = m_pMultiLanguage->ConvertString(&dwMode, nSrcCodePage, nCodePage, reinterpret_cast<uint8_t*>(const_cast<char*>(m_pBuffer.String)), &size_s, reinterpret_cast<uint8_t*>(pDestBuffer), &size_d);
  return hr == S_OK;
}
