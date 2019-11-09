#include "stdshit.h"
#include "findfile.h"
#include "findcore.h"

namespace FindList
{

xarray<FileInfo> list;
const size_t sizeMax_total = 1500*1024*1024;
const size_t sizeMax_file = 256*1024*1024;
const u32 alloc_overhead = sizeof(FileInfo)+16;
size_t alloc_total;
size_t total_size;

void reset(void) { list.Clear();
	alloc_total = 0; total_size = 0; }
	
static __stdcall
size_t find_cb(int code, FindFiles_t& ff)
{
	if(!code) {
		// check memory allocation
		if(ff.nFileSize > sizeMax_file) {	code = 2; goto RET; }
		u32 allocSize = alloc_overhead + ALIGN(ff.relName().slen+1, 7) 
			+	ALIGN(u32(ff.nFileSize), 7);
		if((alloc_total+allocSize) > sizeMax_total) {
			if(alloc_total > (sizeMax_total-sizeMax_total/32))
				return 1; code = 3; goto RET; }

		// load the file
		auto data = loadFile(ff.fName);
		if(!data) { code = (data.len == -1U) ? 1 : 3; goto RET; }
		alloc_total += allocSize; total_size += data.len;
		list.push_back(ff.relName().xdup(), data);
	}
	
	RET: return progress(ff.relName(), code);
}
	
size_t find(cch* path)
{
	reset();
	return findFiles(path, 
		FF_ERR_CB, 0, find_cb);
}
}

namespace FastFind
{

#define icmpPair_tst(r, ch, w) ({ bool _ret_; \
	asm("xor %b2, %b1; and %h2, %b1" : "=@ccz"(_ret_),\
		"+a"(u8(ch)), "+"#r(w)); ; _ret_; })

cstr_<u16> icmpPair_gen(cch* str)
{
	// allocate the string
	cstr_<u16> ned; 
	ned.slen = strlen(str);
	ned.alloc_();

	// build the string
	for(size_t i = 0;;) {
		byte ch = str[i]; int w = ch+0xFF00;
		if(isAlpha(ch)) w &= ~0x2020; VARFIX(w); 
		ned[i] = w; i++; if(!u8(w)) break; }
	return ned;
}

char* icmpPair_find(char* kaystack, 
	size_t haylen, cstr_<u16> needle)
{
	// check needle
	if(haylen < needle.slen) return 0;
	haylen -= needle.slen;
	u16 w0 = needle[0];
	u16* ned = needle+1;
	
	// comparison loop
	char* hayend = kaystack+haylen;
	do { byte ch = *kaystack; kaystack++;
		if(icmpPair_tst(c, ch, w0)) {
			for(int i = 0;;) { VARFIX(i);
				ch = kaystack[i]; u16 w = ned[i];
				i++; if(!u8(w)) { VARFIX(kaystack);
					return kaystack-1; }
				if(!icmpPair_tst(d, ch, w)) break;
			}}
	} while(kaystack <= hayend);
	
	return 0;
}

}
