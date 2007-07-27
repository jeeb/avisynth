string: DB "Hello world!"
HelloWorld:
	push	string
	call	printf
	add		esp, 4

	ret