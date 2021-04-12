#ifndef TUNSTALL_H
#define TUNSTALL_H

#ifdef __cplusplus
extern "C" {
#endif

u16 tunstall_comp(const u8 *in, u8 *out, const u16 len);
void tunstall_decomp(const u8 *in, u8 *out, const u16 outlen);

#ifdef __cplusplus
}
#endif

#endif
