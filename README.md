# Compressor based on [Canonical Huffman code](https://en.wikipedia.org/wiki/Canonical_Huffman_code)
# Build
```gcc huff.c -lm -o huff``` 
# Usage
```
$ ./huff file
```
# Compression example
Input file:
```
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.
```
Results:
```
Source file size in bytes - 446
Encoded size in bytes - 233
```