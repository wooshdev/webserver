/* can't think of anything clever to store these right now, other than as string literals.
 * the problem is that, the length of the bits, and the bits themselve must be stored in a 
 * fast but compact format. strings are easy to represent, so i'll do this for now.
 */
const char *http2_huffman_tree[256] = {
	"1111111111000",
	"11111111111111111011000",
	"1111111111111111111111100010",				/* 1 */
	"1111111111111111111111100011",
	"1111111111111111111111100100",
	"1111111111111111111111100101",
	"1111111111111111111111100110",
	"1111111111111111111111100111",
	"1111111111111111111111101000",
	"111111111111111111101010",
	"111111111111111111111111111100",			/* 10 */
	"1111111111111111111111101001",
	"1111111111111111111111101010"
	"111111111111111111111111111101",
	"1111111111111111111111101011",
	"1111111111111111111111101100",
	"1111111111111111111111101101",
	"1111111111111111111111101110",
	"1111111111111111111111101111",
	"1111111111111111111111110000",
	"1111111111111111111111110001",				/* 20 */
	"1111111111111111111111110010",
	"111111111111111111111111111110",
	"1111111111111111111111110011",
	"1111111111111111111111110100",
	"1111111111111111111111110101",
	"1111111111111111111111110110",
	"1111111111111111111111110111",
	"1111111111111111111111111000",
	"1111111111111111111111111001",
	"1111111111111111111111111010",				/* 30 */
	"1111111111111111111111111011",
	"010100",
	"1111111000",
	"1111111001", 
	"111111111010",
	"1111111111001",
	"010101",
	"11111000",
	"11111111010",
	"1111111010",													/* 40 */
	"1111111011",
	"11111001",
	"11111111011",
	"11111010",
	"010110",
	"010111",
	"011000",
	"00000",
	"00001",
	"00010",															/* 50 */
	"011001",
	"011010",
	"011011",
	"011100",
	"011101",
	"011110",
	"011111",
	"1011100",
	"11111011",
	"11111111111110",											/* 60 */
	"100000",
	"111111111011",
	"1111111100",
	"1111111111010",
	"100001",
	"1011101",
	"1011110",              
	"1011111",
	"1100000",
	"1100001",														/* 70 */
	"1100010",
	"1100011",
	"1100100",
	"1100101",
	"1100110",
	"1100111",
	"1101000",
	"1101001",
	"1101010",
	"1101011",														/* 80 */
	"1101100",
	"1101101",
	"1101110",
	"1101111",
	"1110000",
	"1110001",
	"1110010",
	"11111100",
	"1110011",
	"11111101",														/* 90 */
	"1111111111011",
	"1111111111111110000",
	"1111111111100",
	"11111111111100",
	"100010",
	"111111111111101",
	"00011",
	"100011",
	"00100",
	"100100",															/* 100 */
	"00101",
	"100101",
	"100110",
	"100111",
	"00110",
	"1110100",
	"1110101",
	"101000",
	"101001",
	"101010",															/* 110 */
	"00111",
	"101011",
	"1110110",
	"101100",
	"01000",
	"01001",
	"101101",
	"1110111",
	"1111000",
	"1111001",														/* 120 */
	"1111010",
	"1111011",
	"111111111111110",
	"11111111100",
	"11111111111101",
	"1111111111101",
	"1111111111111111111111111100",
	"11111111111111100110",
	"1111111111111111010010",
	"11111111111111100111",								/* 130 */
	"11111111111111101000",
	"1111111111111111010011",
	"1111111111111111010100",
	"1111111111111111010101",
	"11111111111111111011001",
	"1111111111111111010110",
	"11111111111111111011010",
	"11111111111111111011011",
	"11111111111111111011100",
	"11111111111111111011101",						/* 140 */
	"11111111111111111011110",
	"111111111111111111101011",
	"11111111111111111011111",
	"111111111111111111101100",
	"111111111111111111101101",
	"1111111111111111010111",
	"11111111111111111100000",
	"111111111111111111101110",
	"11111111111111111100001",
	"11111111111111111100010",						/* 150 */
	"11111111111111111100011",
	"11111111111111111100100",
	"111111111111111011100",
	"1111111111111111011000",
	"11111111111111111100101",
	"1111111111111111011001",
	"11111111111111111100110",
	"11111111111111111100111",
	"111111111111111111101111",
	"1111111111111111011010",							/* 160 */
	"111111111111111011101",
	"11111111111111101001",
	"1111111111111111011011",
	"1111111111111111011100",
	"11111111111111111101000",
	"11111111111111111101001",
	"111111111111111011110",
	"11111111111111111101010",
	"1111111111111111011101",
	"1111111111111111011110",							/* 170 */
	"111111111111111111110000",
	"111111111111111011111",
	"1111111111111111011111",
	"11111111111111111101011",
	"11111111111111111101100",
	"111111111111111100000",
	"111111111111111100001",
	"1111111111111111100000",
	"111111111111111100010",
	"11111111111111111101101",						/* 180 */
	"1111111111111111100001",
	"11111111111111111101110",
	"11111111111111111101111",
	"11111111111111101010",
	"1111111111111111100010",
	"1111111111111111100011",
	"1111111111111111100100",
	"11111111111111111110000",
	"1111111111111111100101",
	"1111111111111111100110",							/* 190 */
	"11111111111111111110001",
	"11111111111111111111100000",
	"11111111111111111111100001",
	"11111111111111101011",
	"1111111111111110001",
	"1111111111111111100111",
	"11111111111111111110010",
	"1111111111111111101000",
	"1111111111111111111101100",
	"11111111111111111111100010",					/* 200 */
	"11111111111111111111100011",
	"11111111111111111111100100",
	"111111111111111111111011110",
	"111111111111111111111011111",
	"11111111111111111111100101",
	"111111111111111111110001",
	"1111111111111111111101101",
	"1111111111111110010",
	"111111111111111100011",
	"11111111111111111111100110",					/* 210 */
	"111111111111111111111100000",
	"111111111111111111111100001",
	"11111111111111111111100111",
	"111111111111111111111100010",
	"111111111111111111110010",
	"111111111111111100100",
	"111111111111111100101",
	"11111111111111111111101000",
	"11111111111111111111101001",
	"1111111111111111111111111101",				/* 220 */
	"111111111111111111111100011",
	"111111111111111111111100100",
	"111111111111111111111100101",
	"11111111111111101100",
	"111111111111111111110011",
	"11111111111111101101",
	"111111111111111100110",
	"1111111111111111101001",
	"111111111111111100111",
	"111111111111111101000",							/* 230 */
	"11111111111111111110011",
	"1111111111111111101010",
	"1111111111111111101011",
	"1111111111111111111101110",
	"1111111111111111111101111",
	"111111111111111111110100",
	"111111111111111111110101",
	"11111111111111111111101010",
	"11111111111111111110100",
	"11111111111111111111101011",					/* 240 */
	"111111111111111111111100110",
	"11111111111111111111101100",
	"11111111111111111111101101",
	"111111111111111111111100111",
	"111111111111111111111101000",
	"111111111111111111111101001",
	"111111111111111111111101010",
	"111111111111111111111101011",
	"1111111111111111111111111110",
	"111111111111111111111101100",				/* 250 */
	"111111111111111111111101101",
	"111111111111111111111101110",
	"111111111111111111111101111",
	"111111111111111111111110000",
	"11111111111111111111101110",
	"111111111111111111111111111111",			/* EOS */
};
