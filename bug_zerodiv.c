void api_putchar(int c);
void api_putstr0(char* str);
void api_end(void);

void HariMain(void) {
    int a = 24, b = 3, c = 0;
    c = a / b;
    api_putchar(c - '0');
    b = 0;
    c = a / b;
    api_putchar(c - '0');
    api_end();
}