﻿#if 1
#ifndef WCHAR
#define WCHAR unsigned short
#endif
#endif

//From: ARIB STD-B24 (6.3) VOLUME1 Table7-19
static const WCHAR GaijiTable[]={
	//row 90
	0x26CC, 0x26CD, 0x2757, 0x26CF, 0x26D0, 0x26D1, 0xE0CF, 0x26D2, 0x26D5, 0x26D3,
	0x26D4, 0xE0D4, 0xE0D5, 0xE0D6, 0xE0D7, 0xE0D8, 0xE0D9, 0xE0DA, 0xE0DB, 0x26D6,
	0x26D7, 0x26D8, 0x26D9, 0x26DA, 0x26DB, 0x26DC, 0x26DD, 0x26DE, 0x26DF, 0x26E0,
	0x26E1, 0x2B55, 0x3248, 0x3249, 0x324A, 0x324B, 0x324C, 0x324D, 0x324E, 0x324F,
	0xE0F1, 0xE0F2, 0xE0F3, 0xE0F4, 0x2491, 0x2492, 0x2493, 0xE0F8, 0xE0F9, 0xE0FA,
	0xE0FB, 0xE0FC, 0xE0FD, 0xE0FE, 0xE0FF, 0xE180, 0xE181, 0xE182, 0xE183, 0xE184,
	0xE185, 0xE186, 0xE187, 0x2B1B, 0x2B24, 0xE18A, 0xE18B, 0xE18C, 0xE18D, 0xE18E,
	0x26BF, 0xE190, 0xE191, 0xE192, 0xE193, 0xE194, 0xE195, 0xE196, 0xE197, 0xE198,
	0xE199, 0xE19A, 0x3299, 0xE19C, 0xE19D, 0xE19E, 0xE19F, 0xE1A0, 0xE1A1, 0xE1A2,
	0xE1A3, 0xE1A4, 0xE1A5, 0xE1A6,
	//row 91
	0x26E3, 0x2B56, 0x2B57, 0x2B58, 0x2B59, 0x2613, 0x328B, 0x3012, 0x26E8, 0x3246,
	0x3245, 0x26E9, 0x0FD6, 0x26EA, 0x26EB, 0x26EC, 0x2668, 0x26ED, 0x26EE, 0x26EF,
	0x2693, 0x2708, 0x26F0, 0x26F1, 0x26F2, 0x26F3, 0x26F4, 0x26F5, 0xE1C3, 0x24B9,
	0x24C8, 0x26F6, 0xE1C7, 0xE1C8, 0xE1C9, 0xE1CA, 0xE1CB, 0x26F7, 0x26F8, 0x26F9,
	0x26FA, 0xE1D0, 0x260E, 0x26FB, 0x26FC, 0x26FD, 0x26FE, 0xE1D6, 0x26FF, 0xE1D8,
	0xE1D9, 0xE1DA, 0xE1DB, 0xE1DC, 0xE1DD, 0xE1DE, 0xE1DF, 0xE1E0, 0xE1E1, 0xE1E2,
	0xE1E3, 0xE1E4, 0xE1E5, 0xE1E6, 0xE1E7, 0xE1E8, 0xE1E9, 0xE1EA, 0xE1EB, 0xE1EC,
	0xE1ED, 0xE1EE, 0xE1EF, 0xE1F0, 0xE1F1, 0xE1F2, 0xE1F3, 0xE1F4, 0xE1F5, 0xE1F6,
	0xE1F7, 0xE1F8, 0xE1F9, 0xE1FA, 0xE1FB, 0xE1FC, 0xE1FD, 0xE1FE, 0xE1FF, 0xE280,
	0xE281, 0xE282, 0xE283, 0xE284,
	//row 92
	0x27A1, 0x2B05, 0x2B06, 0x2B07, 0x2B2F, 0x2B2E, 0xE28B, 0xE28C, 0xE28D, 0xE28E,
	0x33A1, 0x33A5, 0x339D, 0x33A0, 0x33A4, 0xE28F, 0x2488, 0x2489, 0x248A, 0x248B,
	0x248C, 0x248D, 0x248E, 0x248F, 0x2490, 0xE290, 0xE291, 0xE292, 0xE293, 0xE294,
	0xE295, 0xE296, 0xE297, 0xE298, 0xE299, 0xE29A, 0xE29B, 0xE29C, 0xE29D, 0xE29E,
	0xE29F, 0x3233, 0x3236, 0x3232, 0x3231, 0x3239, 0x3244, 0x25B6, 0x25C0, 0x3016,
	0x3017, 0x27D0, 0x00B2, 0x00B3, 0xE2A4, 0xE2A5, 0xE2A6, 0xE2A7, 0xE2A8, 0xE2A9,
	0xE2AA, 0xE2AB, 0xE2AC, 0xE2AD, 0xE2AE, 0xE2AF, 0xE2B0, 0xE2B1, 0xE2B2, 0xE2B3,
	0xE2B4, 0xE2B5, 0xE2B6, 0xE2B7, 0xE2B8, 0xE2B9, 0xE2BA, 0xE2BB, 0xE2BC, 0xE2BD,
	0xE2BE, 0xE2BF, 0xE2C0, 0xE2C1, 0xE2C2, 0xE2C3, 0xE2C6, 0x3247, 0xE2C4, 0xE2C5,
	0x213B, 0xE2C7, 0xE2C8, 0xE2C9,
	//row 93
	0x322A, 0x322B, 0x322C, 0x322D, 0x322E, 0x322F, 0x3230, 0x3237, 0x337E, 0x337D,
	0x337C, 0x337B, 0x2116, 0x2121, 0x3036, 0x26BE, 0xE2CD, 0xE2CE, 0xE2CF, 0xE2D0,
	0xE2D1, 0xE2D2, 0xE2D3, 0xE2D4, 0xE2D5, 0xE2D6, 0xE2D7, 0xE2D8, 0xE2D9, 0xE2DA,
	0xE2DB, 0xE2DC, 0xE2DD, 0xE2DE, 0xE2DF, 0xE2E0, 0xE2E1, 0xE2E2, 0x2113, 0x338F,
	0x3390, 0x33CA, 0x339E, 0x33A2, 0x3371, 0xE2E3, 0xE2E4, 0x00BD, 0x2189, 0x2153,
	0x2154, 0x00BC, 0x00BE, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215A, 0x2150,
	0x215B, 0x2151, 0x2152, 0x2600, 0x2601, 0x2602, 0x26C4, 0x2616, 0x2617, 0x26C9,
	0x26CA, 0x2666, 0x2665, 0x2663, 0x2660, 0x26CB, 0x2A00, 0x203C, 0x2049, 0x26C5,
	0x2614, 0x26C6, 0x2603, 0x26C7, 0x26A1, 0x26C8, 0xE2F8, 0x269E, 0x269F, 0x266C,
	0xE2FB, 0xE2FC, 0xE2FD, 0xE2FE,
	//row 94
	0x2160, 0x2161, 0x2162, 0x2163, 0x2164, 0x2165, 0x2166, 0x2167, 0x2168, 0x2169,
	0x216A, 0x216B, 0x2470, 0x2471, 0x2472, 0x2473, 0x2474, 0x2475, 0x2476, 0x2477,
	0x2478, 0x2479, 0x247A, 0x247B, 0x247C, 0x247D, 0x247E, 0x247F, 0x3251, 0x3252,
	0x3253, 0x3254, 0xE383, 0xE384, 0xE385, 0xE386, 0xE387, 0xE388, 0xE389, 0xE38A,
	0xE38B, 0xE38C, 0xE38D, 0xE38E, 0xE38F, 0xE390, 0xE391, 0xE392, 0xE393, 0xE394,
	0xE395, 0xE396, 0xE397, 0xE398, 0xE399, 0xE39A, 0xE39B, 0xE39C, 0x3255, 0x3256,
	0x3257, 0x3258, 0x3259, 0x325A, 0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465,
	0x2466, 0x2467, 0x2468, 0x2469, 0x246A, 0x246B, 0x246C, 0x246D, 0x246E, 0x246F,
	0x2776, 0x2777, 0x2778, 0x2779, 0x277A, 0x277B, 0x277C, 0x277D, 0x277E, 0x277F,
	0x24EB, 0x24EC, 0x325B, 0xE3A6,
	//row 85
	0x3402, 0xE081, 0x4EFD, 0x4EFF, 0x4F9A, 0x4FC9, 0x509C, 0x511E, 0x51BC, 0x351F,
	0x5307, 0x5361, 0x536C, 0x8A79, 0xE084, 0x544D, 0x5496, 0x549C, 0x54A9, 0x550E,
	0x554A, 0x5672, 0x56E4, 0x5733, 0x5734, 0xFA10, 0x5880, 0x59E4, 0x5A23, 0x5A55,
	0x5BEC, 0xFA11, 0x37E2, 0x5EAC, 0x5F34, 0x5F45, 0x5FB7, 0x6017, 0xFA6B, 0x6130,
	0x6624, 0x66C8, 0x66D9, 0x66FA, 0x66FB, 0x6852, 0x9FC4, 0x6911, 0x693B, 0x6A45,
	0x6A91, 0x6ADB, 0xE08A, 0xE08B, 0xE08C, 0x6BF1, 0x6CE0, 0x6D2E, 0xFA45, 0x6DBF,
	0x6DCA, 0x6DF8, 0xFA46, 0x6F5E, 0x6FF9, 0x7064, 0xFA6C, 0xE08E, 0x7147, 0x71C1,
	0x7200, 0x739F, 0x73A8, 0x73C9, 0x73D6, 0x741B, 0x7421, 0xFA4A, 0x7426, 0x742A,
	0x742C, 0x7439, 0x744B, 0x3EDA, 0x7575, 0x7581, 0x7772, 0x4093, 0x78C8, 0x78E0,
	0x7947, 0x79AE, 0x9FC6, 0x4103,
	//row 86
	0x9FC5, 0x79DA, 0x7A1E, 0x7B7F, 0x7C31, 0x4264, 0x7D8B, 0x7FA1, 0x8118, 0x813A,
	0xFA6D, 0x82AE, 0x845B, 0x84DC, 0x84EC, 0x8559, 0x85CE, 0x8755, 0x87EC, 0x880B,
	0x88F5, 0x89D2, 0x8AF6, 0x8DCE, 0x8FBB, 0x8FF6, 0x90DD, 0x9127, 0x912D, 0x91B2,
	0x9233, 0x9288, 0x9321, 0x9348, 0x9592, 0x96DE, 0x9903, 0x9940, 0x9AD9, 0x9BD6,
	0x9DD7, 0x9EB4, 0x9EB5, 0xE096, 0xE097, 0xE098, 0xE099, 0xE09A, 0xE09B, 0xE09C,
	0xE09D, 0xE09E, 0xE09F, 0xE0A0, 0xE0A1, 0xE0A2, 0xE0A3, 0xE0A4, 0xE0A5, 0xE0A6,
	0xE0A7, 0xE0A8, 0xE0A9, 0xE0AA, 0xE0AB, 0xE0AC, 0xE0AD, 0xE0AE, 0xE0AF, 0xE0B0,
	0xE0B1, 0xE0B2, 0xE0B3, 0xE0B4, 0xE0B5, 0xE0B6, 0xE0B7, 0xE0B8, 0xE0B9, 0xE0BA,
	0xE0BB, 0xE0BC, 0xE0BD, 0xE0BE, 0xE0BF, 0xE0C0, 0xE0C1, 0xE0C2, 0xE0C3, 0xE0C4,
	0xE0C5, 0xE0C6, 0xE0C7, 0xE0C8,
};

#if 1
#include <stdio.h>
int main(void)
{
	FILE *fp = fopen("Gaiji.txt", "wb");
	if( fp ){
		unsigned char bom[] = {0xFF, 0xFE};
		if( fwrite(bom, 1, 2, fp) != 2 ) return 1;

		int le = 1;
		le = *(char*)&le ? 1 : 0;
		int i = 0;
		for( ; i<sizeof(GaijiTable)/2; i++ ){
			if( fwrite((char*)GaijiTable+i*2+(le?0:1), 1, 1, fp) != 1 ) return 1;
			if( fwrite((char*)GaijiTable+i*2+(le?1:0), 1, 1, fp) != 1 ) return 1;
		}
		printf("Done.\n");
		fclose(fp);
	}
	return 0;
}
#endif
