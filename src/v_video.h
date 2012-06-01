#ifndef __V_VIDEO_H__
#define __V_VIDEO_H__

// 16-bit Lookup Table
extern BYTE RGB32k[32][32][32];
void GenerateLookupTables();

// Returns the closest color to the one desired. String
// should be of the form "rr gg bb".
int V_GetColorFromString (const DWORD *palette, const char *colorstring);
// Similar to above, but can handle names
int V_GetColor (const DWORD *palette, const char *str);

#endif /* __V_VIDEO_H__ */
