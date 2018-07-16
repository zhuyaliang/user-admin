CC = gcc  
MAINC = main1.c user-info.c user-password.c user-share.c user-base.c user-face.c user-list.c user-admin.c
EXEC = style 
CFLAGS = `pkg-config --cflags --libs gtk+-3.0`
	$(CC)  $(MAINC)  -o $(EXEC) $(CFLAGS) -lpwquality -laccountsservice -lgnome-desktop-3 -lcrypt
clean:
	rm $(EXEC) -rf
