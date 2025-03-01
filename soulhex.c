#include <windows.h>
#include <stdio.h>

#define ID_FILE_OPEN 1
#define VIEW_HEX 2
#define VIEW_BINARY 3
#define VIEW_ASCII 4

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void OpenFileDialog(HWND);
void LoadFile(HWND, const char *);
void AskFileView(HWND);
void DisplayFileContent(HWND, HDC, const char *, long, int, int);

char loadedFile[MAX_PATH] = "";
int currentViewMode = VIEW_HEX;
char *fileBuffer = NULL;
long fileSize = 0;
int scrollPosX = 0; // Horizontal scroll position
int scrollPosY = 0; // Vertical scroll position
int lineHeight = 16; // Height of each line in pixels
int charWidth = 8;   // Width of each character in pixels

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "HexViewerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindow("HexViewerClass", "Hex Viewer", WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
                             CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                             NULL, NULL, hInstance, NULL);
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int) msg.wParam;
}

void OpenFileDialog(HWND hwnd) {
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "All Files\0*.*\0";
    ofn.lpstrFile = loadedFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    
    if (GetOpenFileName(&ofn)) {
        AskFileView(hwnd);
    }
}

void AskFileView(HWND hwnd) {
    int choice = MessageBox(hwnd, "Choose File View:\nYes - Hex View\nNo - Binary View\nCancel - ASCII View", "File View Mode", MB_YESNOCANCEL);
    
    if (choice == IDYES) {
        currentViewMode = VIEW_HEX;
    } else if (choice == IDNO) {
        currentViewMode = VIEW_BINARY;
    } else {
        currentViewMode = VIEW_ASCII;
    }
    LoadFile(hwnd, loadedFile);
}

void LoadFile(HWND hwnd, const char *filePath) {
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        MessageBox(hwnd, "Failed to open file.", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    rewind(file);
    
    fileBuffer = (char *)malloc(fileSize + 1);
    fread(fileBuffer, 1, fileSize, file);
    fclose(file);
    
    fileBuffer[fileSize] = '\0';
    
    // Reset scroll positions
    scrollPosX = 0;
    scrollPosY = 0;
    
    // Update scrollbars
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = fileSize / 16; // Number of lines
    si.nPage = 1; // Number of lines visible at once
    si.nPos = 0;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    
    si.nMax = 100; // Arbitrary horizontal scroll range
    si.nPage = 10; // Arbitrary horizontal page size
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
    
    InvalidateRect(hwnd, NULL, TRUE);
}

void DisplayFileContent(HWND hwnd, HDC hdc, const char *buffer, long size, int scrollX, int scrollY) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    int y = 10;
    int x = 10 - scrollX * charWidth;
    
    for (long i = scrollY * 16; i < size && y < rect.bottom; i += 16) {
        char lineBuffer[256] = "";
        if (currentViewMode == VIEW_HEX) {
            sprintf(lineBuffer, "%08X: ", i);
            for (int j = 0; j < 16 && i + j < size; j++) {
                sprintf(lineBuffer + strlen(lineBuffer), "%02X ", (unsigned char)buffer[i + j]);
            }
            sprintf(lineBuffer + strlen(lineBuffer), "  ");
            for (int j = 0; j < 16 && i + j < size; j++) {
                sprintf(lineBuffer + strlen(lineBuffer), "%c", (buffer[i + j] >= 32 && buffer[i + j] <= 126) ? buffer[i + j] : '.');
            }
        } else if (currentViewMode == VIEW_BINARY) {
            sprintf(lineBuffer, "%08X: ", i);
            for (int j = 0; j < 16 && i + j < size; j++) {
                for (int k = 7; k >= 0; k--) {
                    sprintf(lineBuffer + strlen(lineBuffer), "%d", (buffer[i + j] >> k) & 1);
                }
                sprintf(lineBuffer + strlen(lineBuffer), " ");
            }
        } else if (currentViewMode == VIEW_ASCII) {
            sprintf(lineBuffer, "%08X: ", i);
            for (int j = 0; j < 16 && i + j < size; j++) {
                sprintf(lineBuffer + strlen(lineBuffer), "%c", (buffer[i + j] >= 32 && buffer[i + j] <= 126) ? buffer[i + j] : '.');
            }
        }
        TextOut(hdc, x, y, lineBuffer, strlen(lineBuffer));
        y += lineHeight;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HMENU hMenu = CreateMenu();
            AppendMenu(hMenu, MF_STRING, ID_FILE_OPEN, "Open File");
            SetMenu(hwnd, hMenu);
            DragAcceptFiles(hwnd, TRUE);
        } break;
        
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_FILE_OPEN) {
                OpenFileDialog(hwnd);
            }
            break;
        
        case WM_DROPFILES: {
            DragQueryFile((HDROP)wParam, 0, loadedFile, MAX_PATH);
            DragFinish((HDROP)wParam);
            AskFileView(hwnd);
        } break;
        
        case WM_SIZE: {
            // Update scrollbars based on window size
            SCROLLINFO si;
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_PAGE | SIF_RANGE;
            
            // Vertical scrollbar
            si.nMin = 0;
            si.nMax = fileSize / 16;
            si.nPage = (HIWORD(lParam) - 20) / lineHeight;
            SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
            
            // Horizontal scrollbar
            si.nMax = 100;
            si.nPage = LOWORD(lParam) / charWidth;
            SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
        } break;
        
        case WM_VSCROLL:
        case WM_HSCROLL: {
            SCROLLINFO si;
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_ALL;
            GetScrollInfo(hwnd, (msg == WM_VSCROLL) ? SB_VERT : SB_HORZ, &si);
            
            int oldPos = (msg == WM_VSCROLL) ? scrollPosY : scrollPosX;
            switch (LOWORD(wParam)) {
                case SB_LINEUP: si.nPos--; break;
                case SB_LINEDOWN: si.nPos++; break;
                case SB_PAGEUP: si.nPos -= si.nPage; break;
                case SB_PAGEDOWN: si.nPos += si.nPage; break;
                case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
            }
            si.fMask = SIF_POS;
            SetScrollInfo(hwnd, (msg == WM_VSCROLL) ? SB_VERT : SB_HORZ, &si, TRUE);
            GetScrollInfo(hwnd, (msg == WM_VSCROLL) ? SB_VERT : SB_HORZ, &si);
            
            if (msg == WM_VSCROLL) {
                scrollPosY = si.nPos;
            } else {
                scrollPosX = si.nPos;
            }
            InvalidateRect(hwnd, NULL, TRUE);
        } break;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (fileBuffer) {
                DisplayFileContent(hwnd, hdc, fileBuffer, fileSize, scrollPosX, scrollPosY);
            }
            EndPaint(hwnd, &ps);
        } break;
        
        case WM_DESTROY:
            if (fileBuffer) free(fileBuffer);
            PostQuitMessage(0);
            break;
        
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}