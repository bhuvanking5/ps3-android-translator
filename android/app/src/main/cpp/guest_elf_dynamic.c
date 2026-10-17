#include "guest_elf_dynamic.h"
#include <limits.h>
#include <string.h>
#define EH 64u
#define PH 56u
#define ET_EXEC 2u
#define EM_PPC64 21u
#define PT_LOAD 1u
#define PT_DYNAMIC 2u
#define PF_W 2u
#define DT_NULL 0
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9
#define DYNENT 16u
#define RELAENT 24u
#define MAX_PHDRS 128u
#define MAX_DYN 4096u
typedef struct { uint64_t off,va,fs,ms; uint32_t flags; } seg;
static uint16_t b16(const uint8_t*p){return (uint16_t)(((uint16_t)p[0]<<8)|p[1]);}
static uint32_t b32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static uint64_t b64(const uint8_t*p){return ((uint64_t)b32(p)<<32)|b32(p+4);}
static int range_ok(size_t n,uint64_t o,uint64_t l){return o<=(uint64_t)n&&l<=(uint64_t)n-o;}
static int v2f(const seg*s,size_t n,uint64_t va,uint64_t len,uint64_t*out){
 for(size_t i=0;i<n;i++) if(va>=s[i].va&&va-s[i].va<=s[i].fs&&len<=s[i].fs-(va-s[i].va)){
  if(s[i].off>UINT64_MAX-(va-s[i].va))return 0;
  *out=s[i].off+(va-s[i].va);return 1;
 } return 0;
}
ps3_guest_dynamic_status ps3_guest_elf_parse_rela(const uint8_t*f,size_t n,ps3_guest_relocation*out,size_t cap,size_t*count){
 if(!f||!out||!count||n<EH||cap>PS3_GUEST_ELF_DYNAMIC_MAX_RELOCS)return PS3_GUEST_DYNAMIC_INVALID_ARGUMENT;
 *count=0;
 if(memcmp(f,"\x7f""ELF",4)||f[4]!=2||f[5]!=2||f[6]!=1||b16(f+18)!=EM_PPC64||b32(f+20)!=1)return PS3_GUEST_DYNAMIC_UNSUPPORTED;
 if(b16(f+16)!=ET_EXEC)return PS3_GUEST_DYNAMIC_UNSUPPORTED;
 uint64_t po=b64(f+32);uint16_t pe=b16(f+54),pn=b16(f+56);
 if(pe<PH||pn>MAX_PHDRS||po>n||(uint64_t)pn>((uint64_t)n-po)/pe)return PS3_GUEST_DYNAMIC_BAD_FORMAT;
 seg ss[MAX_PHDRS];size_t sn=0;uint64_t doff=0,dsz=0;int hd=0;
 for(uint16_t i=0;i<pn;i++){
  const uint8_t*p=f+(size_t)(po+(uint64_t)i*pe);uint32_t t=b32(p);
  if(t==PT_LOAD){seg*s=&ss[sn++];s->flags=b32(p+4);s->off=b64(p+8);s->va=b64(p+16);s->fs=b64(p+32);s->ms=b64(p+40);
   if(s->fs>s->ms||!range_ok(n,s->off,s->fs)||s->va>UINT64_MAX-s->ms)return PS3_GUEST_DYNAMIC_BAD_FORMAT;
  }else if(t==PT_DYNAMIC){if(hd)return PS3_GUEST_DYNAMIC_BAD_FORMAT;hd=1;doff=b64(p+8);dsz=b64(p+32);
   if(!dsz||dsz%DYNENT||!range_ok(n,doff,dsz)||dsz/DYNENT>MAX_DYN)return PS3_GUEST_DYNAMIC_BAD_FORMAT;}
 }
 if(!hd)return PS3_GUEST_DYNAMIC_BAD_FORMAT;
 uint64_t rv=0,rs=0,re=0;int hr=0,hs=0,he=0,term=0;
 for(uint64_t o=0;o<dsz;o+=DYNENT){const uint8_t*p=f+(size_t)(doff+o);int64_t tag=(int64_t)b64(p);uint64_t v=b64(p+8);
  if(tag==DT_NULL){term=1;break;}if(tag==DT_RELA){rv=v;hr=1;}else if(tag==DT_RELASZ){rs=v;hs=1;}else if(tag==DT_RELAENT){re=v;he=1;}
 }
 if(!term)return PS3_GUEST_DYNAMIC_BAD_FORMAT;
 if(!hr&&!hs&&!he)return PS3_GUEST_DYNAMIC_OK;
 if(!hr||!hs||!he||re!=RELAENT||rs%RELAENT)return PS3_GUEST_DYNAMIC_BAD_FORMAT;
 uint64_t nc=rs/RELAENT;if(nc>cap||nc>PS3_GUEST_ELF_DYNAMIC_MAX_RELOCS)return PS3_GUEST_DYNAMIC_TOO_MANY_RELOCS;
 uint64_t ro=0;if(rs&&!v2f(ss,sn,rv,rs,&ro))return PS3_GUEST_DYNAMIC_OUT_OF_BOUNDS;
 for(size_t i=0;i<(size_t)nc;i++){
  const uint8_t*p=f+(size_t)(ro+i*RELAENT);uint64_t target=b64(p),info=b64(p+8),raw=b64(p+16);
  uint32_t type=(uint32_t)info,sym=(uint32_t)(info>>32);int writable=0;
  if(sym||(type!=PS3_PPC64_R_ADDR32&&type!=PS3_PPC64_R_REL32))return PS3_GUEST_DYNAMIC_UNSUPPORTED;
  if(target>UINT32_MAX-3u||(target&3u))return PS3_GUEST_DYNAMIC_OUT_OF_BOUNDS;
  for(size_t j=0;j<sn;j++)if((ss[j].flags&PF_W)&&target>=ss[j].va&&target-ss[j].va<=ss[j].ms&&4u<=ss[j].ms-(target-ss[j].va)){writable=1;break;}
  if(!writable)return PS3_GUEST_DYNAMIC_OUT_OF_BOUNDS;
  out[i].type=type;out[i].target_address=(uint32_t)target;out[i].symbol_value=0;
  out[i].addend=raw<=(uint64_t)INT64_MAX?(int64_t)raw:-1-(int64_t)(UINT64_MAX-raw);
 }
 *count=(size_t)nc;return PS3_GUEST_DYNAMIC_OK;
}
