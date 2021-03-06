#include "stdshit.h"
#include "findfile.h"
#include "findcore.h"

namespace FindList
{

xarray<FileInfo> list;
static size_t sizeMax_total = 1500*1024*1024;
const size_t sizeMax_file = 256*1024*1024;
const u32 alloc_overhead = sizeof(FileInfo)+16;
static size_t alloc_total;
size_t total_size;

void reset(void) { list.Clear();
	alloc_total = 0; total_size = 0; 

	if(sizeMax_total) {
		MEMORYSTATUS ms; GlobalMemoryStatus(&ms);
		sizeMax_total = ms.dwAvailVirtual - 256*1024*1024;
	}
}
	
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



static 
int pathcmp(cch* str1, cch* str2) 
{
	// compare the strings
	byte ch1, ch2; int ret;
	for(;; str1++, str2++) {
		ch1 = toUpper(*str1);
		ch2 = toUpper(*str2);
		if(ret=ch1-ch2) break;
		if(!ch1) return ret;
	}
	
	// sort files first
	for(; *str1; str1++) {
		if(isPathSep(*str1)){ ret += 0x1000; break; }}
	for(; *str2; str2++) {
		if(isPathSep(*str2)){ ret -= 0x1000; break; }}
	return ret;
}

static
int FileInfo_cmp(const FileInfo& a, const FileInfo& b) {
	return pathcmp(a.name.data, b.name.data); }

size_t find(cch* path)
{
	reset();
	size_t ret = findFiles(path, 
		FF_ERR_CB|FF_DIR_AFT, 0, find_cb);
	qsort(list, FileInfo_cmp);
	return ret;
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
	if(!str) return {0,0};
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

static
byte* isWordChar(byte* ch) {
	switch(*ch) { case '_':
	case 'a'...'z': case 'A'...'Z':
	case '0'...'9': return ch; }
	return 0;
}

static
bool cmprev(byte* pos, 
	byte* base, const char* str)
{
	int len = strlen(str);
	pos = (pos+1) - len;
	if(pos < base) return false;
	return !memcmp(pos, str, len);
}

static
bool isSpcChar(byte* ch) {
	return *ch <= ' '; }
	
static
bool isNewLn(byte* ch) { return 
	(*ch == '\n')||(*ch == '\r'); }

bool cdef_check(xarray<byte> data, byte* str)
{
	// exclude operators
	byte* f = str;
	while(--f >= data.data) {
		if(isSpcChar(f)||(*f == '\\')) continue;
		if(!isWordChar(f) || cmprev(f, data.data, "return"))
			return false; break;
	}
	
	// exclude strings
	for(f = str; (f > data.data)&&(!isNewLn(f-1)); f--);
	for(byte state = 0;; f++) { if(f >= str) return !state;
		if(!state) { if(is_one_of(*f, '\'', '\"')) state = *f; }
		else { if(*f == '\\') f++; ei(*f == state) state = 0; }
	}
	
	return true;
}

bool inc_check(byte* base, byte* f)
{
	// locate include
	byte* i = f;
	while((--i >= base)&&(!isNewLn(i)))
		if(!strnicmp((char*)i, "include"))
			goto FOUND_INC; return false; 
	FOUND_INC: VARFIX(i);
	
	// check string
	for(i += 7; isSpcChar(i); i++) { if(i >= f) return false; }
	if(!is_one_of(*i, '\'', '\"', '<')) return false;
	for(byte state = *i; (i++, 1);) { if(i >= f) return true;
		if(*f == '\\') f++; ei(*f == state) return false; }
	__builtin_trap();
}

char* IcmpFind::find(xarray<byte> data, int flags)
{
	byte* end = data.end();
	byte* pos = data;
	
FIND_NEXT:
	byte* f = (byte*)icmpPair_find(
		(char*)pos, end-pos, needle);
	pos = f+needle.slen;
		
	if(f) {
	
		// check bounds	
		if(flags & 1) {
			if(!(((--f < data.data)||(!isWordChar(f)))
			&&((pos >= end)||(!isWordChar(pos)))))
				goto FIND_NEXT;
		}
		
		// check def
		if(flags & 2) {
			if(!cdef_check(data, f))
				goto FIND_NEXT;
		}
		
		// check includes
		if(flags & 4) {
			if(inc_check(data, f))
				goto FIND_NEXT;
		}
	}
	
	return (char*)f;
}
}
