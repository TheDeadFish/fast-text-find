#pragma once

namespace FindList
{
	struct FileInfo { 
		xstr name;
		xArray<byte> data; 
	};

	__stdcall size_t find(cch* path);
	__stdcall void reset(void);
	__stdcall size_t progress(cch* path, int ec);
	extern xarray<FileInfo> list;
	extern size_t total_size;
};

namespace FastFind
{

	cstr_<u16> icmpPair_gen(cch* str);
	char* icmpPair_find(char* kaystack, 
		size_t haylen, cstr_<u16> needle);
		
	struct IcmpFind {
		IcmpFind(cch* str) {needle.init(icmpPair_gen(str)); }
		char* find(xarray<byte> data, int flags);
		Cstr_<u16> needle; 
	};
}
