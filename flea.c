/*
----------------------------------------------------------------------------
FLEA: Fast Little Encryption Algorithm, by Bob Jenkins 2002
This is free.  Public domain.  Use it for any purpose you wish.

This is not the best file encryption tool in the world, or the easiest
to use, or the most secure, or the fastest.  It has no redeeming qualities.
I recommend ignoring it and finding something else to play with.  Pay no
attention to this little program here.
----------------------------------------------------------------------------
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void usage()
{
  fprintf(stderr, "This program encrypts and decrypts files.\n\n");
  fprintf(stderr, "If salt is '.', the input and output file names will be\n");
  fprintf(stderr, "used as the salt in sorted order.\n\n");
  fprintf(stderr, "To encrypt:\n");
  fprintf(stderr, "  copy myfile.txt q\n");
  fprintf(stderr, "  flea\n");
  fprintf(stderr, "    input file : q\n");
  fprintf(stderr, "    output file: myfile.flea\n");
  fprintf(stderr, "    salt       : .\n");
  fprintf(stderr, "    password   : <secret password>\n");
  fprintf(stderr, "To decrypt:\n");
  fprintf(stderr, "  flea\n");
  fprintf(stderr, "    input file : q\n");
  fprintf(stderr, "    output file: myfile.flea\n");
  fprintf(stderr, "    salt       : .\n");
  fprintf(stderr, "    password   : <secret password>\n");
  fprintf(stderr, "  examine q\n");
  fprintf(stderr, "  delete q\n");
  fprintf(stderr, "When encrypting, try decrypting before deleting myfile.txt\n");
  fprintf(stderr, "to make sure you entered the password right.\n");
}

extern char *getpass();

#define  SIZE 64
#define  SEEDSIZE (SIZE/2)
typedef  unsigned long  uint32;

typedef struct flea {
  uint32 *r;        /* results given to the user */
  uint32  m[SIZE];  /* big secret internal state */
  uint32  b,c,d;    /* more secret state */
  uint32  count;    /* more state, guarantees no short cycles */
} flea;

void rand( flea *x)
{
  uint32 i, *r=x->r, *m=x->m, a, b=x->b+(++x->count), c=x->c, d=x->d;
  for (i=0; i<SIZE; ++i) {
    a = m[b % SIZE];
    m[b % SIZE] = d;
    d = (c<<19) + (c>>13) + b;
    c = b ^ m[i];
    b = a + d;
    r[i] ^= c;
  }
  x->b=b; x->c=c; x->d=d;
}

void init( flea *x, uint32 *results, uint32 *seed, int length)
{
  int i, j;
  x->b = 0x01234567;  x->c = 0x89abcdef;  x->d = 0x31415927;  x->count = 0;
  if (length > SEEDSIZE) {
    fprintf(stderr, "error: password must be shorter than %d\n", SEEDSIZE);
    exit(3);
  }
  for (i=0, j=0; i<SIZE; ++i, ++j) {
    if (j==length) j = 0;
    x->m[i] = seed[j];
  }
  x->r = results;
  for (i=0; i<3; ++i) rand(x);                         /* mix the state well */
}

/* This encrypts/decrypts a file and sends it to stdout */
void driver(char *password, size_t len, FILE *fi, FILE *fo)
{
  uint32  r[SIZE], seed[SEEDSIZE];
  uint32  i, j;
  flea    x;

  /* convert the password to a seed and initialize flea */
  for (i=0, j=0; i<SEEDSIZE; ++i) {
    if (j==len) break;
    seed[i] = password[j++];
    if (j==len) {++i; break;}
    seed[i] += (password[j++]<<8);
    if (j==len) {++i; break;}
    seed[i] += (password[j++]<<16);
    if (j==len) {++i; break;}
    seed[i] += (password[j++]<<24);
  }
  init(&x, r, seed, i);

  /* XOR the input with flea to produce the output */
  while (!feof(fi)) {
    i = fread(r, 1, SIZE*sizeof(uint32), fi);
    rand(&x);
    fwrite(r, 1, i, fo);
  }
}

/* this interprets inputs */
int main(int argc, char **argv)
{
  /* interpret the password */
  if (argc == 1) {
    char    file_in[256], file_out[256], salt[256], *secret, password[512];
    size_t  len;
    FILE   *fi, *fo;
    fprintf(stderr,  "  input file : ");
    scanf("%256s", file_in);
    fprintf(stderr,  "  output file: ");
    scanf("%256s", file_out);
    fprintf(stderr,  "  salt       : ");
    scanf("%256s", salt);
    getchar();
    secret = getpass("  password   : ");

    len = 0;
    if (strcmp(salt, ".") != 0) {
      memcpy(password, salt, strlen(salt));
      len = strlen(salt);
    }
    else if (strcmp(file_in, file_out) > 0) {
      memcpy(password, file_in, strlen(file_in));
      memcpy(password+strlen(file_in), file_out, strlen(file_out));
      len = strlen(file_in) + strlen(file_out);
    }
    else if (strcmp(file_in, file_out) < 0) {
      memcpy(password, file_out, strlen(file_out));
      memcpy(password+strlen(file_out), file_in, strlen(file_in));
      len = strlen(file_in) + strlen(file_out);
    }
    
    if (strcmp(file_in, file_out) == 0) {
      fprintf(stderr, "input and output files must be different\n");
      exit(4);
    }
    memcpy(password+len, secret, strlen(secret)+1);
    len = strlen(password)+1;

    fi = fopen(file_in, "rb");
    if (!fi) {
      fprintf(stderr, "could not open file %s\n", file_in);
      exit(1);
    }
    fo = fopen(file_out, "wb");
    if (!fo) {
      fprintf(stderr, "could not open file %s\n", file_out);
      exit(3);
    }

    driver(password, len, fi, fo);
    fclose(fi);
    fclose(fo);
  } else {
    usage();
    exit(2);
  }
  return 0;
}

