#ifndef _GBA_FSIO_H_
#define _GBA_FSIO_H_

#ifdef GBA_FSIO_CORE
#define GBA_FSIO_IWRAM_CALL_LENGTH
#else
#define GBA_FSIO_IWRAM_CALL_LENGTH ,long_call
#endif

#define GBA_FSIO_IWRAM_FUNC(func) __attribute__ ((section(".iwram.text") GBA_FSIO_IWRAM_CALL_LENGTH)) func

/* Experimental write pre-caching */
// #define GAMEBOY_WCACHE   1

#define GBA_MAXFILES 4
#define DI_MAX       512

typedef struct dcache_s {
    int base;
    int nbytes;
    unsigned char *data;
} dcache_t;

extern const char *unixv5;
extern char gsbuf[];
extern long fpos[];
extern int filestable[];
extern int di;
extern dcache_t D[];

void gba_tty_init(void);
void gba_tty_print(const char *s);
void gba_tty_inittext(void);
void gba_tty_setxy(int x, int y);
void gba_tty_scroll(void);
void gba_tty_putc(int c);
int gba_puts(char *s);
int gba_putsnl(char *s);
int gba_kbd_read(int d, void *buf, int nbytes);

GBA_FSIO_IWRAM_FUNC(int gba_getc(FILE *stream));
GBA_FSIO_IWRAM_FUNC(size_t gba_fread(void *ptr, size_t size, size_t nmemb, FILE *stream));
GBA_FSIO_IWRAM_FUNC(long gba_ftell(FILE *stream));
GBA_FSIO_IWRAM_FUNC(int gba_fseek(FILE *stream, long offset, int whence));
GBA_FSIO_IWRAM_FUNC(size_t gba_fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream));
GBA_FSIO_IWRAM_FUNC(void gba_rewind(FILE *stream));

void gba_abort(char *msg);
void *gba_calloc(size_t nmemb, size_t size);
void gba_clearerr(FILE *stream);
int gba_fclose(FILE *stream);
int gba_ferror(FILE *stream);
char *gba_fgets(char *cptr, int size, FILE *stream);
void gba_fsio_init(void);
FILE *gba_fopen(const char *path, const char *mode);
int gba_fputs(const char *str, FILE *stream);
int gba_puts(char *str);

/* suppress warnings */
#undef calloc
#undef clearerr
#undef getc
#undef fclose
#undef ferror
#undef fgets
#undef fopen
#undef fputs
#undef fread
#undef fseek
#undef fwrite
#undef rewind
#undef printf

#define calloc gba_calloc
#define clearerr(F)
#define getc gba_getc
#define fclose gba_fclose
#define ferror(F) 0
#define fgets gba_fgets
#define fopen gba_fopen
#define fputs gba_fputs
#define fread gba_fread
#define fseek gba_fseek
#define ftell gba_ftell
#define fwrite gba_fwrite
#define rewind gba_rewind

/* Critical */
#define printf(fmt, ...) sprintf(gsbuf, fmt, ## __VA_ARGS__), gba_puts(gsbuf)

#endif /* _GBA_FSIO_H_ */
