void api_putchar(int c);
void api_end(void);

void HariMain(void){
    char a[100];
    a[10] = 'A';
    api_putchar(a[10]);
    a[100] = 'B';
    api_putchar(a[100]);
    a[1000] = 'C';
    api_putchar(a[1000]);
    a[10000] = 'D';
    api_putchar(a[10000]);
    a[100000] = 'E';
    api_putchar(a[100000]);
    a[1000000] = 'F';
    api_putchar(a[1000000]);
    a[10000000] = 'G';
    api_putchar(a[10000000]);
    a[100000000] = 'H';
    api_putchar(a[100000000]);
    a[1000000000] = 'I';
    api_putchar(a[1000000000]);
    a[2147483647] = 'J';
    api_putchar(a[2147483647]);
    api_end();
}