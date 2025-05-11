putsn(str,len)
{
        __asm("mov eax,4");
        __asm("mov ebx,1");
        __asm("mov ecx,[ebp+12]");
        __asm("mov edx,[ebp+8]");
        __asm("int 0x80");
}

main()
{
        int a;
        a = 1;
        if (a > 1)
        {
                return 2;
        }
        putsn("hello, world!\n",14);
        return a;
}