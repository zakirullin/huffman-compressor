#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned int uint;
typedef unsigned char byte;
typedef int bool;

#define FILE_NAME_ARG 1
#define MIN_ARGS 2

#define ALP_SIZE 256
#define ALP_LAST (ALP_SIZE - 1)
#define BITS_PER_BYTE 8
#define MAX_LENGTH 32

#define FETCH_BIT(buf, n) ((byte)(buf[n / BITS_PER_BYTE] << (n % BITS_PER_BYTE)) >> (BITS_PER_BYTE - 1))
#define GET_N_LAST_BITS(code, n) (code & (byte)(pow(2, n) - 1))

int get_file_size(FILE *fp)
{
    int size;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    return size;
}

byte *read_file(FILE *fp)
{
    int file_size = get_file_size(fp);

    byte* buf = malloc(sizeof(byte) * file_size);
    fread(buf, sizeof(byte), file_size, fp);

    return buf;
}

int *calc_freq(byte *buf, int size)
{
    int* freq = calloc(sizeof(int), ALP_SIZE);

    int i;
    for (i = 0; i < size; i++) {
        freq[buf[i]]++; 
    }

    return freq;
}

// Order aware sorting
void sort(int *freq, int *symb)
{
    int *pos = malloc(sizeof(int) * ALP_SIZE); 
    int i;
    for (i = 0; i < ALP_SIZE; i++) {
        if (symb != NULL) {
            symb[i] = i;
            pos[i] = i;
        }
    }

    // Insertion sort
    int j;
    for (i = 0; i < ALP_SIZE; i++) {
        int value = freq[i];
        j = i - 1;
        // Find suitable j position
        while ((j >= 0) && (freq[j] > value)) {
            freq[j + 1] = freq[j];  
            if (symb != NULL) {
                pos[j + 1] = pos[j];
                symb[pos[j + 1]]++;
            }
            j--;
        }
        freq[j + 1] = value;
        if (symb != NULL) {
            pos[j + 1] = i;
            symb[i] = j + 1; 
        }
    }   
}

void calc_len(int *freq)
{
    // First phase - set parent pointers 
    int leaf = 0; // Leaf index
    int node = 0; // Node index
    int root; // Root node 
    for (root = 0; root < ALP_SIZE - 1; root++) {
        // Select first node 
        if ((leaf > ALP_SIZE - 1) || ((node < root) && (freq[node] < freq[leaf]))) {
            // Select a node
            freq[root] = freq[node];
            freq[node] = root; // Set pointer to parent
            node++;
        } else {
            // Select a leaf
            freq[root] = freq[leaf];
            leaf++;
        }

        // Add second node 
        if ((leaf > ALP_SIZE - 1) || ((node < root) && (freq[node] < freq[leaf]))) {
            // Select a node
            freq[root] += freq[node];
            freq[node] = root; // Set pointer to parent
            node++;
        } else {
            // Select a leaf
            freq[root] += freq[leaf];
            leaf++;
        }
    }

    // Second phase - set node depth 
    freq[ALP_SIZE - 2] = 0; // Root depth

    int i;
    for (i = ALP_SIZE - 3; i >= 0; i--) {
        freq[i] = freq[freq[i]] + 1;
    }

    // Third phase - set leaf depth
    int depth = 0;
    int nodes_amount = 0;
    int nodes_and_leaves_amount = 1;
    int vacant = ALP_SIZE - 1;
    int node_index = ALP_SIZE - 2;
    while (nodes_and_leaves_amount > 0) {
        while ((node_index >= 0) && (freq[node_index] == depth)) {
            nodes_amount++;
            node_index--;
        }

        while (nodes_and_leaves_amount > nodes_amount) {
            freq[vacant] = depth;
            vacant--;
            nodes_and_leaves_amount--;
        }

        nodes_and_leaves_amount = 2 * nodes_amount;
        nodes_amount = 0;
        depth++;
    }
}

// Sort symbol by two keys:
// 1. By code length
// 2. By alphabetical order
void sort_len(int *freq, int *symb)
{
    // Pack length
    int i;
    for (i = 0; i < ALP_SIZE; i++) {
        freq[symb[i]] *= ALP_SIZE;
        freq[symb[i]] += i;
    }

    sort(freq, NULL);

    // Unpack length
    for (i = 0; i < ALP_SIZE; i++) {
        symb[i] = freq[i] % ALP_SIZE;
        freq[i] = freq[i] / ALP_SIZE;

        if (freq[i] >= MAX_LENGTH) {
            printf("Maximum bit-length reached\n!");
            exit(1);
        }
    }
}

void print_binary(int val, int len)
{
    int i;
    for (i = (32 - len); i < 32; i++) {
        if (val & (1 << (31 - i))) {
            printf("1");
        } else {
            printf("0");
        }
    }
    printf("\n");
}

void create_codebook(int *len, int *symb, int **codebook, int **codelen)
{
    *codebook = calloc(sizeof(int), ALP_SIZE);
    *codelen = calloc(sizeof(int), ALP_SIZE);

    int root;
    int leaf;
    int level;
    int i;
    int *nodes = calloc(sizeof(int), MAX_LENGTH);
    for(root = 0, i = ALP_LAST, level = len[ALP_LAST]; level > 0; level--) {
        leaf = 0;
        while ((i >= 0) && (len[i] == level)) 
        { 
            leaf++; 
            i--; 
        }

        nodes[level] = root; 
        root = (root + leaf) >> 1;
    }

    int count;
    for (i = 0, count = 0; i < ALP_SIZE; i++) {
        level = len[i]; 
        (*codebook)[symb[i]] = nodes[level] + count;
        (*codelen)[symb[i]] = len[i];
        printf("symb - %c(%i): ", symb[i], symb[i]);
        print_binary(nodes[level] + count, len[i]);

        if ((i != ALP_SIZE - 1) && len[i] < len[i + 1]) {
            count = 0;
        } else {
            count++;
        }
    }
}

void encode_file(int *codebook, int *codelen, byte *buf, int buf_size, byte **obuf, int *obuf_size, int *freq, int *symb, int *packed_tree)
{
    *obuf = calloc(sizeof(byte), buf_size);
    *obuf_size = 0;

    int free_bits = BITS_PER_BYTE;
    int i;
    int shift;
    int pending_bits;
    for (i = 0; i < buf_size; i++) {
        pending_bits = codelen[buf[i]];

        while (pending_bits != 0) {
            if (pending_bits >= free_bits) {
                (*obuf)[*obuf_size] <<= free_bits;
                (*obuf)[(*obuf_size)++] |= GET_N_LAST_BITS(codebook[buf[i]] >> (pending_bits - free_bits), pending_bits);
                pending_bits -= free_bits;
                free_bits = BITS_PER_BYTE;
            } else {
                (*obuf)[*obuf_size] <<= pending_bits; 
                (*obuf)[*obuf_size] |= GET_N_LAST_BITS(codebook[buf[i]], pending_bits); 
                free_bits -= pending_bits; 
                if (i == buf_size - 1) {
                    (*obuf)[(*obuf_size)++] <<= free_bits;
                }
                pending_bits = 0;
            }
        }
    }   

    FILE *ofp = fopen("encoded", "w+");
    // Pack tree    
    fwrite(packed_tree, sizeof(int), ALP_SIZE, ofp);

    fwrite(*obuf, sizeof(byte), *obuf_size, ofp);
    fclose(ofp);
}

void encode(char *filename)
{
    // Create buffer 
    FILE *fp = fopen(filename, "r");
    byte *buf;
    buf = read_file(fp);
    int buf_size = get_file_size(fp);

    // Sort frequences
    int *freq = calc_freq(buf, buf_size);
    int *symb = calloc(sizeof(int), ALP_SIZE);
    sort(freq, symb);

    // Sort len
    calc_len(freq);
    int *packed_tree = calloc(sizeof(int), ALP_SIZE);
    int i;
    for (i = 0; i < ALP_SIZE; i++) {
        packed_tree[i] = freq[symb[i]];
    }
    sort_len(freq, symb);

    // Create codebook
    int *codebook;
    int *codelen;
    create_codebook(freq, symb, &codebook, &codelen);

    // Encode
    byte *obuf;
    int obuf_size;
    encode_file(codebook, codelen, buf, buf_size, &obuf, &obuf_size, freq, symb, packed_tree);
    
    printf("Source file size in bytes - %i\n", buf_size);
    printf("Encoded size in bytes - %i\n", obuf_size);

    // Free memory
    fclose(fp);
    free(buf);
    free(obuf);
    free(freq);
    free(symb);
    free(codebook);
    free(codelen);
}

// nodes - first leaf level
// offs - internal nodes per level
void create_decode_tables(int *len, int *symb, int *nodes, int *offs, byte *buf, int buf_size)
{
    int root;
    int leaf;
    int level;
    int i;
    for(root = 0, i = ALP_LAST, level = len[ALP_LAST]; level > 0; level--) {
        leaf = 0;
        while ((i >= 0) && (len[i] == level)) 
        { 
            leaf++; 
            i--; 
        }

        nodes[level] = root; 
        root = (root + leaf) >> 1;
        if (leaf) {
            offs[level] = i + 1; 
        }
    }
}

void decode_file(int *symb, byte *buf, int buf_size, int *nodes, int *offs)
{
    byte *obuf = calloc(sizeof(byte), buf_size * 5);
    int obuf_size = 0;
    int length;
    int code;
    uint fetched_bits = 0;
    while ((fetched_bits / BITS_PER_BYTE) < (buf_size)) {
        length = 0;
        code = 0;
        do {
            code <<= 1;
            code |= FETCH_BIT(buf, fetched_bits); 
            fetched_bits++;
            length++;
        } while ((code < nodes[length]) && (fetched_bits < (buf_size * BITS_PER_BYTE)));

        if (fetched_bits < (buf_size * BITS_PER_BYTE)) {
            obuf[obuf_size++] = symb[offs[length] + code - nodes[length]];
        }
    }

    FILE *fp = fopen("decoded", "w+");
    fwrite(obuf, sizeof(byte), obuf_size, fp);

    fclose(fp);
    free(obuf);
}

void decode()
{
    // Read file
    FILE *fp = fopen("encoded", "r");
    int buf_size = get_file_size(fp) - (ALP_SIZE * sizeof(int));
    int *freq = calloc(sizeof(int), ALP_SIZE); 
    fread(freq, sizeof(int), ALP_SIZE, fp);
    byte *buf = calloc(sizeof(int), buf_size);  
    fread(buf, sizeof(byte), buf_size, fp);

    // Create symb array
    int *symb = calloc(sizeof(int), ALP_SIZE);
    int i;
    for (i = 0; i < ALP_SIZE; i++) {
        symb[i] = i;
    }

    // Calc decode tables
    sort_len(freq, symb);
    int *nodes = calloc(sizeof(int), MAX_LENGTH); 
    int *offs = calloc(sizeof(int), MAX_LENGTH);
    create_decode_tables(freq, symb, nodes, offs, buf, buf_size);

    decode_file(symb, buf, buf_size, nodes, offs);
}

int main(int argc, char **argv)
{
    if (argc < MIN_ARGS) {
        printf("You must provide a file name!\n");
        
        return EXIT_FAILURE;
    }

    encode(argv[FILE_NAME_ARG]);
    decode();

    return EXIT_SUCCESS;
}
