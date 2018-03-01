#include "lib7zip.h"

#ifdef S_OK
#undef S_OK
#endif

#ifndef INITGUID
#define INITGUID
#endif

#include "CPP/7zip/Archive/IArchive.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/Common/MyCom.h"
#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/IPassword.h"
#include "CPP/7zip/Common/FileStreams.h"

#include "HelperFuncs.h"

inline bool VARIANT_BOOLToBool(VARIANT_BOOL value)
{ return (value != VARIANT_FALSE); }

HRESULT ReadProp(
						GetHandlerPropertyFunc getProp,
						GetHandlerPropertyFunc2 getProp2,
						UInt32 index, PROPID propID, NWindows::NCOM::CPropVariant &prop)
{
	if (getProp2)
		return getProp2(index, propID, &prop);;
	return getProp(propID, &prop);
}

HRESULT ReadBoolProp(
							GetHandlerPropertyFunc getProp,
							GetHandlerPropertyFunc2 getProp2,
							UInt32 index, PROPID propID, bool &res)
{
	NWindows::NCOM::CPropVariant prop;

	RINOK(ReadProp(getProp, getProp2, index, propID, prop));
	if (prop.vt == VT_BOOL)
		res = VARIANT_BOOLToBool(prop.boolVal);
	else if (prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

HRESULT ReadStringProp(
							  GetHandlerPropertyFunc getProp,
							  GetHandlerPropertyFunc2 getProp2,
							  UInt32 index, PROPID propID, wstring &res)
{
	NWindows::NCOM::CPropVariant prop;

	RINOK(ReadProp(getProp, getProp2, index, propID, prop));
	if (prop.vt == VT_BSTR)
		res = prop.bstrVal;
	else if (prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

void SplitString(const wstring &srcString, WStringArray &destStrings)
{
	destStrings.clear();
	wstring s;
	size_t len = srcString.length();
	if (len == 0)
		return;
	for (size_t i = 0; i < len; i++)
	{
		wchar_t c = srcString[i];
		if (c == L' ')
		{
			if (!s.empty())
			{
				destStrings.push_back(s);
				s.clear();
			}
		}
		else
			s += c;
	}
	if (!s.empty())
		destStrings.push_back(s);
}

HRESULT GetCoderClass(GetMethodPropertyFunc getMethodProperty, UInt32 index,
							 PROPID propId, GUID & clsId, bool &isAssigned)
{
	NWindows::NCOM::CPropVariant prop;
	isAssigned = false;
	RINOK(getMethodProperty(index, propId, &prop));
	if (prop.vt == VT_BSTR)
	{
		isAssigned = true;
		clsId = *(const GUID *)prop.bstrVal;
	}
	else if (prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

HRESULT GetMethodPropertyString(GetMethodPropertyFunc getMethodProperty, UInt32 index,
                             PROPID propId, wstring & val)
{
    NWindows::NCOM::CPropVariant prop;
    RINOK(getMethodProperty(index, propId, &prop));
    if (prop.vt == VT_BSTR)
    {
        val = prop.bstrVal;
    }
    else if (prop.vt != VT_EMPTY)
        return E_FAIL;
    return S_OK;
}

HRESULT GetMethodPropertyGUID(GetMethodPropertyFunc getMethodProperty, UInt32 index,
                             PROPID propId, GUID & val)
{
    NWindows::NCOM::CPropVariant prop;
    RINOK(getMethodProperty(index, propId, &prop));
    if (prop.vt == VT_BSTR)
    {
        val = *(const GUID *)prop.bstrVal;
    }
    else if (prop.vt != VT_EMPTY)
        return E_FAIL;
    return S_OK;
}


#ifdef _WIN32
inline wchar_t MyCharUpper(wchar_t c)
{ return (wchar_t)(unsigned int)(UINT_PTR)CharUpperW((LPWSTR)(UINT_PTR)(unsigned int)c); }
#else
inline wchar_t MyCharUpper(wchar_t c)
{ return toupper(c); }
#endif

int MyStringCompareNoCase(const wchar_t *s1, const wchar_t *s2)
{
	for (;;)
	{
		wchar_t c1 = *s1++;
		wchar_t c2 = *s2++;
		if (c1 != c2)
		{
			wchar_t u1 = MyCharUpper(c1);
			wchar_t u2 = MyCharUpper(c2);
			if (u1 < u2) return -1;
			if (u1 > u2) return 1;
		}
		if (c1 == 0) return 0;
	}
}

void NormalizeDirPathPrefix(wstring & dirPath)
{
	if (dirPath.empty())
		return;
	if (dirPath.rfind(wchar_t(kDirDelimiter)) != dirPath.length() - 1)
		dirPath += wchar_t(kDirDelimiter);
}

HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, wstring &result)
{
	NWindows::NCOM::CPropVariant prop;
	RINOK(archive->GetProperty(index, kpidPath, &prop));
	if(prop.vt == VT_BSTR)
		result = prop.bstrVal;
	else if (prop.vt == VT_EMPTY)
		result.clear();
	else
		return E_FAIL;
	return S_OK;
}

HRESULT GetArchiveItemPath(IInArchive *archive, UInt32 index, const wstring &defaultName, wstring &result)
{
	RINOK(GetArchiveItemPath(archive, index, result));
	if (result.empty())
	{
		result = defaultName;
		NWindows::NCOM::CPropVariant prop;
		RINOK(archive->GetProperty(index, kpidExtension, &prop));
		if (prop.vt == VT_BSTR)
		{
			result += L'.';
			result += prop.bstrVal;
		}
		else if (prop.vt != VT_EMPTY)
			return E_FAIL;
	}
	return S_OK;
}

HRESULT GetArchiveItemFileTime(IInArchive *archive, UInt32 index,
							   const FILETIME &defaultFileTime, FILETIME &fileTime)
{
	NWindows::NCOM::CPropVariant prop;
	RINOK(archive->GetProperty(index, kpidMTime, &prop));
	if (prop.vt == VT_FILETIME)
		fileTime = prop.filetime;
	else if (prop.vt == VT_EMPTY)
		fileTime = defaultFileTime;
	else
		return E_FAIL;
	return S_OK;
}

HRESULT IsArchiveItemProp(IInArchive *archive, UInt32 index, PROPID propID, bool &result)
{
	NWindows::NCOM::CPropVariant prop;
	RINOK(archive->GetProperty(index, propID, &prop));
	if(prop.vt == VT_BOOL)
		result = VARIANT_BOOLToBool(prop.boolVal);
	else if (prop.vt == VT_EMPTY)
		result = false;
	else
		return E_FAIL;
	return S_OK;
}

HRESULT IsArchiveItemFolder(IInArchive *archive, UInt32 index, bool &result)
{
	return IsArchiveItemProp(archive, index, kpidIsDir, result);
}

HRESULT IsArchiveItemAnti(IInArchive *archive, UInt32 index, bool &result)
{
	return IsArchiveItemProp(archive, index, kpidIsAnti, result);
}

UInt64 ConvertPropVariantToUInt64(const PROPVARIANT &prop)
{
	switch (prop.vt)
	{
	case VT_UI1: return prop.bVal;
	case VT_UI2: return prop.uiVal;
	case VT_UI4: return prop.ulVal;
	case VT_UI8: return (UInt64)prop.uhVal.QuadPart;
	default:
#ifndef _WIN32_WCE
		throw 151199;
#else
		return 0;
#endif
	}
}
