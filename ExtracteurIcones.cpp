// ExtracteurIcones.cpp : Defines the entry point for the application.
// Monolithique, Win32 API pur
//
#include "ExtracteurIcones.h"
// IconScanner.cpp
// Monolithique, Win32 API pur, compatible VS6

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")


HINSTANCE   g_hInst = NULL;
HWND        g_hMainWnd = NULL;
HWND        g_hListView = NULL;
HWND        g_hStatusBar = NULL;
HIMAGELIST  g_hImageList = NULL;
UINT        g_uIconCount = 0;   // compteur global d'icônes

#define ID_LISTVIEW         1001
#define ID_STATUSBAR        1002

#define ID_MENU_SCAN_ALL    2001
#define ID_MENU_SCAN_FOLDER 2002
#define ID_MENU_EXIT        2003


struct ICON_INFO
{
    TCHAR szPath[MAX_PATH];
    int   indexInFile;
};
ICON_INFO* g_pIconInfos = NULL;
UINT       g_uIconInfosCapacity = 0;

/// <summary>
/// s'assure que le tableau global de stockage des infos d'icônes a la capacité nécessaire pour stocker les infos d'une nouvelle icône
/// (en doublant la capacité à chaque fois que c'est nécessaire)
/// </summary>
void AjustementCapaciteTableauInfoIcone(UINT needed)
{
    if (needed <= g_uIconInfosCapacity)return;
    UINT newCap = g_uIconInfosCapacity == 0 ? 1024 : g_uIconInfosCapacity * 2;
    while (newCap < needed)newCap *= 2;
    ICON_INFO* pNew = (ICON_INFO*)GlobalAlloc(GPTR, newCap * sizeof(ICON_INFO));
    if (!pNew)return;
    if (g_pIconInfos)
    {
        CopyMemory(pNew, g_pIconInfos, g_uIconInfosCapacity * sizeof(ICON_INFO));
        GlobalFree(g_pIconInfos);
    }
    g_pIconInfos = pNew;
    g_uIconInfosCapacity = newCap;
}
/// <summary>
/// Vérifie si le fichier a une extension .exe ou .dll (sans tenir compte de la casse) 
/// pour décider s'il faut tenter d'en extraire les icônes ou pas
/// </summary>
static BOOL IsExeOrDll(LPCTSTR pszPath)
{
    LPCTSTR ext = PathFindExtension(pszPath);
    if (!ext || !*ext) return FALSE;
    if (lstrcmpi(ext, TEXT(".exe")) == 0) return TRUE;
    if (lstrcmpi(ext, TEXT(".dll")) == 0) return TRUE;
    return FALSE;
}

static void ActualiserBarreEtat()
{
    if (!g_hStatusBar) return;
    TCHAR szText[128];
    wsprintf(szText, TEXT("Icônes : %u"), g_uIconCount);
    SendMessage(g_hStatusBar, SB_SETTEXT, 0, (LPARAM)szText);
}
/// <summary>
/// Ajoute une icône à la zone d'affichage (ListView) et stocke les infos associées (chemin + index dans le fichier) 
/// dans un tableau global pour pouvoir les réutiliser plus tard (ex: lors d'un clic sur l'item pour afficher les détails de l'icône)
/// </summary>
static void AjouterIconeDanZoneAffichage(HICON hIcon, LPCTSTR pszSourceFile, int iconIndexInFile)
{
    if (!g_hImageList || !g_hListView || !hIcon)
        return;

    int imgIndex = ImageList_AddIcon(g_hImageList, hIcon);
    if (imgIndex == -1)
        return;
    g_uIconCount++;
    AjustementCapaciteTableauInfoIcone(g_uIconCount);
    if (g_pIconInfos)
    {
        ICON_INFO* pInfo = &g_pIconInfos[g_uIconCount - 1];
        lstrcpyn(pInfo->szPath, pszSourceFile, MAX_PATH);
        pInfo->indexInFile = iconIndexInFile;
    }
    TCHAR szText[256];
	wsprintf(szText, TEXT("%.8u"), g_uIconCount); // Affiche l'index global de l'icône
    LVITEM lvi;
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_IMAGE;
    lvi.iItem = g_uIconCount - 1; // position = index global - 1
    lvi.iSubItem = 0;
    lvi.pszText = szText;
    lvi.iImage = imgIndex;
    ListView_InsertItem(g_hListView, &lvi);
    ActualiserBarreEtat();
}
/// <summary>
/// ne prends en compte que les fichiers .exe et .dll, et ignore les erreurs d'extraction d'icônes 
/// (ex: pas de ressources d'icônes dans le fichier)
/// et ajoute les icônes extraites à la zone d'affichage
/// </summary>
static void ExtraireIconesDesFichiers(LPCTSTR pszFile)
{
    UINT nIcons = ExtractIconEx(pszFile, -1, NULL, NULL, 0);
    if (nIcons == 0 || nIcons == (UINT)-1) return;
    for (UINT i = 0; i < nIcons; ++i)
    {
        HICON hIconLarge = NULL;
        HICON hIconSmall = NULL;
        UINT got = ExtractIconEx(pszFile, i, &hIconLarge, &hIconSmall, 1);
        if (got == 0 || got == (UINT)-1)continue;
        if (hIconLarge)
        {
            AjouterIconeDanZoneAffichage(hIconLarge, pszFile, i);
            DestroyIcon(hIconLarge);
        }
        if (hIconSmall){DestroyIcon(hIconSmall);}
    }
}
static void AnalyseRecursive(LPCTSTR pszFolder)
{
    TCHAR szSearch[MAX_PATH];
    lstrcpyn(szSearch, pszFolder, MAX_PATH);
    PathAddBackslash(szSearch);
    lstrcat(szSearch, TEXT("*.*"));
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(szSearch, &fd);
    if (hFind == INVALID_HANDLE_VALUE)return;
    do
    {
        if (lstrcmp(fd.cFileName, TEXT(".")) == 0 ||lstrcmp(fd.cFileName, TEXT("..")) == 0){continue;}
        TCHAR szPath[MAX_PATH];
        lstrcpyn(szPath, pszFolder, MAX_PATH);
        PathAddBackslash(szPath);
        lstrcat(szPath, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){AnalyseRecursive(szPath);}
        else
        {
            if (IsExeOrDll(szPath)){ExtraireIconesDesFichiers(szPath);}
        }
    } while (FindNextFile(hFind, &fd));
    FindClose(hFind);
}
static void AnalyserTousLecteursMontes()
{
    ListView_DeleteAllItems(g_hListView);
    if (g_hImageList){ImageList_Destroy(g_hImageList);g_hImageList = NULL;}
    g_hImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 256, 256);
    if (!g_hImageList)return;
    ListView_SetImageList(g_hListView, g_hImageList, LVSIL_NORMAL);
    g_uIconCount = 0;
    ActualiserBarreEtat();
    for (TCHAR drive = TEXT('C'); drive <= TEXT('Z'); ++drive)
    {
        TCHAR szRoot[4] = { drive, TEXT(':'), TEXT('\\'), TEXT('\0') };
        UINT type = GetDriveType(szRoot);
        if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_RAMDISK)
        {
            AnalyseRecursive(szRoot);
        }
    }
    ListView_Arrange(g_hListView, LVA_DEFAULT);
}
/// <summary>
/// Ignore les erreurs d'accès aux dossiers (ex: pas les droits nécessaires pour accéder à un dossier), 
/// et continue l'analyse récursive des autres dossiers
/// ignore aussi les fichiers qui ne sont pas des .exe ou .dll
/// et ajoute les icônes extraites à la zone d'affichage
/// </summary>
/// à noter que l'analyse récursive peut prendre beaucoup de temps si le dossier contient beaucoup de sous-dossiers et de fichiers,
/// et que l'interface ne sera pas responsive pendant ce temps (pas de multithreading dans cette application monolithique),
/// mais c'est voulu pour garder l'exemple simple et se concentrer sur l'extraction d'icônes et la gestion de l'interface
/// dans une application plus complète, j'envisage d'ajouter une barre de progression et de faire l'analyse dans un thread 
/// séparé pour garder l'interface responsive

static void AnalyserDossierRecursivement(LPCTSTR pszFolder)
{
    ListView_DeleteAllItems(g_hListView);
    if (g_hImageList)
    {
        ImageList_Destroy(g_hImageList);
        g_hImageList = NULL;
    }
    g_hImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 256, 256);
    if (!g_hImageList)return;
    ListView_SetImageList(g_hListView, g_hImageList, LVSIL_NORMAL);
    g_uIconCount = 0;
    ActualiserBarreEtat();
    AnalyseRecursive(pszFolder);
    ListView_Arrange(g_hListView, LVA_DEFAULT);
}
static BOOL ParcourirDossiers(HWND hOwner, LPTSTR pszOut, int cchOut)
{
    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.hwndOwner = hOwner;
    bi.lpszTitle = TEXT("Choisissez un dossier à scanner");
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl)return FALSE;
    BOOL ok = SHGetPathFromIDList(pidl, pszOut);
    LPMALLOC pMalloc = NULL;
    if (SUCCEEDED(SHGetMalloc(&pMalloc)) && pMalloc)
    {
        pMalloc->Free(pidl);
        pMalloc->Release();
    }
    if (!ok)return FALSE;
    if ((int)lstrlen(pszOut) >= cchOut)return FALSE;
    return TRUE;
}
static void CreerMenuPrincipal(HWND hWnd)
{
    HMENU hMenuBar = CreateMenu();
    HMENU hMenuFile = CreateMenu();
    AppendMenu(hMenuFile, MF_STRING, ID_MENU_SCAN_ALL, TEXT("&Scanner tous les lecteurs"));
    AppendMenu(hMenuFile, MF_STRING, ID_MENU_SCAN_FOLDER, TEXT("Scanner un &dossier..."));
    AppendMenu(hMenuFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuFile, MF_STRING, ID_MENU_EXIT, TEXT("&Quitter"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hMenuFile, TEXT("&Fichier"));

    SetMenu(hWnd, hMenuBar);
}
static void CreerBarreEtat(HWND hWnd)
{
    g_hStatusBar = CreateWindowEx(0,STATUSCLASSNAME,NULL,WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,0, 0, 0, 0,hWnd,(HMENU)ID_STATUSBAR,g_hInst,NULL);
    if (g_hStatusBar)
    {
        int parts[1] = { -1 };
        SendMessage(g_hStatusBar, SB_SETPARTS, 1, (LPARAM)parts);
        ActualiserBarreEtat();
    }
}
static void AjusterElements(HWND hWnd, int cx, int cy)
{
    int statusHeight = 0;
    if (g_hStatusBar)
    {
        RECT rcStatus;
        SendMessage(g_hStatusBar, WM_SIZE, 0, 0);
        GetWindowRect(g_hStatusBar, &rcStatus);
        statusHeight = rcStatus.bottom - rcStatus.top;
    }
    if (g_hListView)
    {
        MoveWindow(g_hListView, 0, 0, cx, cy - statusHeight, TRUE);
    }
}
void CreerZoneAffichageIcone(HWND hWnd)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    g_hListView = CreateWindowEx(WS_EX_CLIENTEDGE,WC_LISTVIEW,NULL,WS_CHILD | WS_VISIBLE | LVS_ICON | LVS_SHOWSELALWAYS,0, 0,rc.right - rc.left,rc.bottom - rc.top,hWnd,(HMENU)ID_LISTVIEW,g_hInst,NULL);
    if (!g_hListView)return;
    ListView_SetExtendedListViewStyle(g_hListView, LVS_EX_DOUBLEBUFFER);
}
static void MontrerInfosIconeSelectionnee()
{
    if (!g_hListView || !g_pIconInfos) return;
    int iSel = ListView_GetNextItem(g_hListView, -1, LVNI_SELECTED);
    if (iSel < 0)return;
    if ((UINT)iSel >= g_uIconCount)  return;
    ICON_INFO* pInfo = &g_pIconInfos[iSel];
    TCHAR szMsg[1024];
    wsprintf(szMsg,TEXT("Chemin :\r\n%s\r\n\r\nIndex dans le fichier : %d\r\nIndex global : %d"),pInfo->szPath,pInfo->indexInFile,iSel + 1);
    MessageBox(g_hMainWnd, szMsg, TEXT("Détails de l'icône"), MB_OK | MB_ICONINFORMATION);
}
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        INITCOMMONCONTROLSEX icc;
        icc.dwSize = sizeof(icc);
        icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
        InitCommonControlsEx(&icc);
        CreerMenuPrincipal(hWnd);
        CreerBarreEtat(hWnd);
        CreerZoneAffichageIcone(hWnd);
    }
    return 0;
    case WM_SIZE:
    {
        int cx = LOWORD(lParam);
        int cy = HIWORD(lParam);
        AjusterElements(hWnd, cx, cy);
    }
    return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_MENU_SCAN_ALL:
        {
            HCURSOR hOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
            AnalyserTousLecteursMontes();
            SetCursor(hOld);
            MessageBox(hWnd, TEXT("Scan de tous les lecteurs terminé."), TEXT("Info"), MB_OK | MB_ICONINFORMATION);
        }
        return 0;
        case ID_MENU_SCAN_FOLDER:
        {
            TCHAR szFolder[MAX_PATH] = { 0 };
            if (ParcourirDossiers(hWnd, szFolder, MAX_PATH))
            {
                HCURSOR hOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
                AnalyserDossierRecursivement(szFolder);
                SetCursor(hOld);
                MessageBox(hWnd, TEXT("Scan du dossier terminé."), TEXT("Info"), MB_OK | MB_ICONINFORMATION);
            }
        }
        return 0;
        case ID_MENU_EXIT:
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;
    case WM_NOTIFY:
    {
        LPNMHDR pHdr = (LPNMHDR)lParam;
        if (pHdr->hwndFrom == g_hListView)
        {
            if (pHdr->code == NM_DBLCLK)
            {
                MontrerInfosIconeSelectionnee();
                return 0;
            }
        }
    }
    break;
    case WM_DESTROY:
        if (g_hImageList)
        {
            ImageList_Destroy(g_hImageList);
            g_hImageList = NULL;
        }
        if (g_pIconInfos)
        {
            GlobalFree(g_pIconInfos);
            g_pIconInfos = NULL;
            g_uIconInfosCapacity = 0;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
/// <summary>
/// Un point d'entrée classique pour une application Windows, qui crée la fenêtre principale, affiche la fenêtre et lance la boucle de messages
/// il est important de noter que tout le travail de l'application (création de la zone d'affichage, extraction des icônes, etc.) 
/// est déclenché à partir de la procédure de fenêtre (WndProc) en réponse à des messages spécifiques 
/// (ex: WM_CREATE pour l'initialisation, WM_COMMAND pour les actions de menu, etc.)
/// </summary>
int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
    g_hInst = hInstance;
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(wc.hInstance, (LPCSTR)102);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("IconScanner");
    if (!RegisterClass(&wc))return 0;
    g_hMainWnd = CreateWindow(wc.lpszClassName,TEXT("Scanner d'icônes EXE/DLL - Win32    V:1.01"),WS_OVERLAPPEDWINDOW,CW_USEDEFAULT, CW_USEDEFAULT,900, 600,NULL,NULL,hInstance,NULL);
    if (!g_hMainWnd)return 0;
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
