#include "http2hufftree.c"

typedef struct hnode_t {
	struct hnode_t *left;
	struct hnode_t *right;
	/* should only be accessed when left or right are NULL*/
	size_t value;
} hnode_t;

static hnode_t *tree = NULL;

size_t huff_get(char *stream, size_t length) {
	size_t i;
	hnode_t *current = tree;
	
	for (i = 1; i < 8; i++) {
		printf("%i", !!((stream[0] << (i)) & 0x80));
	}
	
	for (i = 0; i < length*sizeof(size_t)*8; i++) {
		if (!current->left || !current->right) {
			if (i != length - 1) {
				printf("[H2] [Huffman] Warning! End of path found, while index != end; index = %zu\n", i);
			}
			return current->value;
		}
		
		/*printf("bit: %c\n", ((stream[i/8] >> i%8) & 1 ? '1' : '0'));*/
		if ((stream[i/8] >> (8-i%8)) & 1) {
			current = current->right;
			/*puts("go right leaf");*/
		} else {
			current = current->left;
			/*puts("go left leaf");*/
		}
	}
	return current->value;
}

int huff_setup() {
	tree = calloc(1, sizeof(hnode_t));
	tree->value = 666;
	if (!tree)
		return 0;
	
	size_t i, j;
	for (i = 0; i < sizeof(http2_huffman_tree) / sizeof (http2_huffman_tree[0]); i++) {
		const char *text = http2_huffman_tree[i];
		hnode_t *current = tree;
		current->value = 648;
		
		for (j = 0; j < strlen(text); j++) {
			if (text[j] == '0') {
				if (!current->left) {
					current = current->left = calloc(1, sizeof(hnode_t));
				} else {
					current = current->left;
				}
			} else if (text[j] == '1') {
				if (!current->right) {
					current = current->right = calloc(1, sizeof(hnode_t));
				} else {
					current = current->right;
				}
			} else {
				puts("Corrupted huffman tree.");
				/* maybe clean already allocated data? */
				return 0;
			}
		}
		
		current->value = i;
	}
	
	/*char test[] = { 0x71 };
	printf("W should appear: '%zu'\n", huff_get(test, sizeof(test)/sizeof(test[0])));*/
	
	return 1;
}
