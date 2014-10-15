#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <windows.h>

HBITMAP g_hbmBall = NULL;
const char g_szClassName[] = "myWindowClass";

// Structure for PPM Pixels
typedef struct {
	unsigned char red, green, blue;
} Pixel;

// Structure for the Image
typedef struct {
	int x, y;
	Pixel **data; // Pixels matrix
} Image;

#define CREATED_BY "PPM IMAGE EDITOR"
#define RGB_TOTAL_COLORS 255
#define IDC_MAIN_EDIT 3001

static Image *readImage(const char *filename)
{
	char buff[16];
	Image *img;
	FILE *fp;
	errno_t err;
	int i, c, rgb_comp_color;

	//open PPM file for reading
	err = fopen_s(&fp, filename, "rb");
	if (err != 0) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		exit(1);
	}

	//read image format
	if (!fgets(buff, sizeof(buff), fp)) {
		perror(filename);
		exit(1);
	}

	//check the image format
	if (buff[0] != 'P' || (buff[1] != '6' && buff[1] != '3')) {
		fprintf(stderr, "Invalid image format (must be 'P6')\n");
		exit(1);
	}

	//alloc memory form image
	img = (Image *)malloc(sizeof(Image));
	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//check for comments
	c = getc(fp);
	while (c == '#') {
		while (getc(fp) != '\n');
		c = getc(fp);
	}

	ungetc(c, fp);
	//read image size information
	if (fscanf_s(fp, "%d %d", &img->x, &img->y) != 2) {
		fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
		exit(1);
	}

	//read rgb component
	if (fscanf_s(fp, "%d", &rgb_comp_color) != 1) {
		fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
		exit(1);
	}

	//check rgb colors
	if (rgb_comp_color != RGB_TOTAL_COLORS) {
		fprintf(stderr, "'%s' does not have 8-bits components\n", filename);
		exit(1);
	}

	while (fgetc(fp) != '\n');
	
	//memory allocation for pixel data
	img->data = (Pixel*)malloc(img->x * sizeof(Pixel*));
	for (i = 0; i < img->x; i++)
		img->data[i] = (Pixel*)malloc(img->y * sizeof(Pixel));

	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//read pixel data from file
	for (i = 0; i < img->y; i++) {
		fread(img->data[i], 3 * img->x, 1, fp);
	}

	fclose(fp);
	return img;
}

void writeImage(const char *filename, Image *img)
{
	FILE *fp;
	errno_t err;
	int i = 0;

	//open file for writing
	err = fopen_s(&fp, filename, "wb");
	if (err != 0) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		exit(1);
	}

	//write the header file
	//image format
	fprintf(fp, "P6\n");

	//comments
	fprintf(fp, "# Created by %s\n", CREATED_BY);

	//image size
	fprintf(fp, "%d %d\n", img->x, img->y);

	// rgb colors
	fprintf(fp, "%d\n", RGB_TOTAL_COLORS);

	// pixel data
	for (i = 0; i < img->y; i++) {
		fwrite(img->data[i], 3 * img->x, 1, fp);
	}
	fclose(fp);
}

void filterChangeColor(Image *img)
{
	int i = 0, j = 0;
	if (img){
		for (i = 0; i<img->x; i++){
			for (j = 0; j<img->y; j++) {
				img->data[i][j].red = RGB_TOTAL_COLORS - img->data[i][j].red;
				img->data[i][j].green = RGB_TOTAL_COLORS - img->data[i][j].green;
				img->data[i][j].blue = RGB_TOTAL_COLORS - img->data[i][j].blue;
			}
		}
	}
}

void filterGaussianBlur(Image *img)
{
	int i = 0, j = 0, k = 0, // indexes
		x = 0, y = 0, // positions
		redAverage = 0,		redTotal = 0,	// red values
		greenAverage = 0,	greenTotal = 0, // green values
		blueAverage = 0,	blueTotal = 0,	// blue values
		pixelLenght = 0, pixelSquare = 0, currentLevel = 0;

	// Gaussian blur level
	int level = 1;

	if (img){
		// Go line by line
		for (i = 0; i<img->x; i++){
			// Go row by row
			for (j = 0; j<img->y; j++) {
				pixelSquare = level * 2 + 1; // one side of the pixels square based on the level
				pixelLenght = pixelSquare * pixelSquare; // total pixels per blur level 
				redTotal = greenTotal = blueTotal = 0; // needs to restart the color sum

				// Now based on the blur level it will get each neighbor pixel
				// Line by line
				for (x = 0; x < pixelSquare; x++) {
					// Row by row
					for (y = 0; y < pixelSquare; y++) {
						// Calculate the exact pixel position we want to get
						int xIndex = i + x - level;
						int yIndex = j + y - level;
						// If pixel position is outside of our matrix then let's go to the next pixel
						if (xIndex < 0 || xIndex >= img->x || yIndex < 0 || yIndex >= img->y)
							continue;

						// Sum the value in a total by color
						redTotal	+= img->data[xIndex][yIndex].red;
						greenTotal	+= img->data[xIndex][yIndex].green;
						blueTotal	+= img->data[xIndex][yIndex].blue;
					}
				}

				// Time to find the average dividing each color result by the total of pixels
				redAverage		= redTotal		/ pixelLenght;
				greenAverage	= greenTotal	/ pixelLenght;
				blueAverage		= blueTotal		/ pixelLenght;

				// Now we do everything again as we did before, but now we fill the colors with the average value
				// Line by line
				for (x = 0; x < pixelSquare; x++) {
					// Row by row
					for (y = 0; y < pixelSquare; y++) {
						// Calculate the exact pixel position we want to get
						int xIndex = i + x - level;
						int yIndex = j + y - level;
						// If pixel position is outside of our matrix then let's go to the next pixel
						if (xIndex < 0 || xIndex >= img->x || yIndex < 0 || yIndex >= img->y)
							continue;

						// Assign the color average value to the pixel
						img->data[xIndex][yIndex].red	= redAverage;
						img->data[xIndex][yIndex].green = greenAverage;
						img->data[xIndex][yIndex].blue	= blueAverage;
					}
				}
			}
		}
	}
}

static void *openImage(HWND hwnd){
	OPENFILENAME ofn;
	char szFileName[MAX_PATH] = "";

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = "Files (*.ppn)\0*.ppm\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "ppm";

	if (GetOpenFileName(&ofn))
	{
		HWND hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
		Image *image = readImage(szFileName);
		filterGaussianBlur(image);
		writeImage(szFileName, image);
		MessageBox(hwnd, "Open Image!", "Error", MB_OK | MB_ICONEXCLAMATION);
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
		case WM_CREATE:
        {
			g_hbmBall = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP1));
			if (g_hbmBall == NULL)
				MessageBox(hwnd, "Could not load Image!", "Error", MB_OK | MB_ICONEXCLAMATION);		
		
            HMENU hMenu, hSubMenu, hIcon, hIconSm;
            hMenu = CreateMenu();
            hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, ID_FILE_OPEN, "&Open File");
			AppendMenu(hSubMenu, MF_STRING, ID_FILE_EXIT, "E&xit");
            AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&File");

            hSubMenu = CreatePopupMenu();
            AppendMenu(hSubMenu, MF_STRING, ID_STUFF_GO, "&About");
            AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&Help");

            SetMenu(hwnd, hMenu);


            hIcon = LoadImage(NULL, "image.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
            if(hIcon)
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            else
                MessageBox(hwnd, "Could not load large icon!", "Error", MB_OK | MB_ICONERROR);

            hIconSm = LoadImage(NULL, "image.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
            if(hIconSm)
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
            else
                MessageBox(hwnd, "Could not load small icon!", "Error", MB_OK | MB_ICONERROR);
        }
        break;
        case WM_COMMAND:
            switch(LOWORD(wParam))
			{
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
				case ID_FILE_OPEN:
				{
					openImage(hwnd);
				}
				break;
				case ID_STUFF_GO:
					MessageBox(hwnd, "About!", "Error", MB_OK | MB_ICONERROR);
				break;
			}
        break;
		case WM_PAINT:
		{
			 BITMAP bm;
			 PAINTSTRUCT ps;

			 HDC hdc = BeginPaint(hwnd, &ps);

			 HDC hdcMem = CreateCompatibleDC(hdc);
			 HBITMAP hbmOld = SelectObject(hdcMem, g_hbmBall);

			 GetObject(g_hbmBall, sizeof(bm), &bm);

			 BitBlt(hdc, 0, 0, bm.bmWidth * 2, bm.bmHeight * 2, hdcMem, -50, -50, SRCCOPY);

			 SelectObject(hdcMem, hbmOld);
			 DeleteDC(hdcMem);

			 EndPaint(hwnd, &ps);
		}
		break;
		case WM_LBUTTONDOWN:  
		{
            char szFileName[MAX_PATH];
            HINSTANCE hInstance = GetModuleHandle(NULL);

            GetModuleFileName(hInstance, szFileName, MAX_PATH);
            MessageBox(hwnd, szFileName, "This program is:", MB_OK | MB_ICONINFORMATION);
        }
		break;              
        case WM_CLOSE:
			DeleteObject(g_hbmBall);
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
	

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "PPM Image Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}