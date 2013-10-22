#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <vd2/vdvideofilt.h>
#pragma GCC diagnostic pop
#include "file_info.hpp"
#include "Renderer.hpp"
#include "virtualdub_dialog.hpp"
#include <windows.h>    // Dialog presentation
#include <commdlg.h>    // File selection dialog
#include "module.hpp" // Module needed for windows
#include "textconv.hpp" // Unicode conversion for dialog in-&output
#include <cstdio>   // _snprintf for description filling

namespace VDub{
    // Filter instance data (allocated in C)
    struct Userdata{
        Renderer *renderer;
        std::string *script;
        bool warnings;
    };
    // Filter configuration dialog message handler
    INT_PTR CALLBACK config_message_handler(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam){
        // Evaluate message
        switch(msg){
            // Dialog initialization
            case WM_INITDIALOG:{
                Userdata* inst_data = reinterpret_cast<Userdata*>(lParam);
                // Set dialog default content
                HWND edit = GetDlgItem(wnd, VDUB_DIALOG_FILENAME);
                SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(utf8_to_utf16(*inst_data->script).c_str()));
                SendMessageW(edit, EM_SETSEL, 0, -1);
                SendMessageW(GetDlgItem(wnd, VDUB_DIALOG_CHECK), BM_SETCHECK, inst_data->warnings, 0);
                // Store userdata to window
                SetWindowLongPtrA(wnd, DWLP_USER, reinterpret_cast<LONG_PTR>(inst_data));
            }break;
            // Dialog action
            case WM_COMMAND:
                // Evaluate action command
                switch(wParam){
                    // '...' button
                    case VDUB_DIALOG_FILENAME_CHOOSE:{
                        wchar_t file[256]; file[0] = '\0';
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
                        OPENFILENAMEW ofn = {0};
#pragma GCC diagnostic pop
                        ofn.lStructSize = sizeof(OPENFILENAMEW);
                        ofn.hwndOwner = wnd;
                        ofn.hInstance = reinterpret_cast<HINSTANCE>(module);
                        ofn.lpstrFilter = L"SSB file (*.ssb)\0*.ssb\0\0";
                        ofn.nFilterIndex = 1;
                        ofn.lpstrFile = file;
                        ofn.nMaxFile = sizeof(file);
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                        // Show file selection dialog
                        if(GetOpenFileNameW(&ofn)){
                            // Save filename input to dialog
                            HWND edit = GetDlgItem(wnd, VDUB_DIALOG_FILENAME);
                            SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(ofn.lpstrFile));
                            SendMessageW(edit, EM_SETSEL, 0, -1);
                        }
                    }break;
                    // 'OK' button
                    case IDOK:{
                        Userdata* inst_data = reinterpret_cast<Userdata*>(GetWindowLongPtrA(wnd, DWLP_USER));
                        // Save dialog content to userdata
                        HWND edit = GetDlgItem(wnd, VDUB_DIALOG_FILENAME);
                        std::wstring filename(static_cast<int>(SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0))+1, L'\0');
                        SendMessageW(edit, WM_GETTEXT, filename.length(), reinterpret_cast<LPARAM>(filename.data()));
                        *inst_data->script = utf16_to_utf8(filename);
                        inst_data->warnings = SendMessageW(GetDlgItem(wnd, VDUB_DIALOG_CHECK), BM_GETCHECK, 0, 0);
                        // Successful end
                        EndDialog(wnd, 0);
                    }break;
                    // 'Cancel' button
                    case IDCANCEL:
                        // Unsuccessful end
                        EndDialog(wnd, 1);
                        break;
                }
                break;
            // Closed dialog with 'X' button
            case WM_CLOSE:
                // Unsuccessful end
                EndDialog(wnd, 1);
                break;
            // Message not handled (continue with default behaviour)
            default:
                return FALSE;
        }
        // Message handled
        return TRUE;
    }
    // Fill description for stringProc & stringProc2
    void fill_description(Userdata* inst_data, char* buf, int maxlen = 128){
        // Fill description buffer with script and warnings information
        _snprintf(buf, maxlen, " Script:\"%s\" - Warnings:%s", inst_data->script->c_str(), inst_data->warnings ? "on" : "off");
    }
    // Filter definition
    VDXFilterDefinition filter_definition = {
        nullptr,	// _next
        nullptr,	// _prev
        nullptr,	// _module

        FILTER_NAME,	// name
        FILTER_DESCRIPTION,	// desc
        FILTER_AUTHOR,	// maker
        nullptr,	// private data
        sizeof(Userdata),	// inst_data_size

        // initProc (Filter initialization)
        [](VDXFilterActivation* fdata, const VDXFilterFunctions*) -> int{
            Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
            inst_data->renderer = nullptr;
            inst_data->script = new std::string;
            inst_data->warnings = true;
            // Success
            return 0;
        },
        // deinitProc (Filter deinitialization)
        [](VDXFilterActivation* fdata, const VDXFilterFunctions*) -> void{
            Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
            delete inst_data->renderer; inst_data->renderer = nullptr;
            delete inst_data->script; inst_data->script = nullptr;
        },
        // runProc (Filter run/frame processing)
        [](const VDXFilterActivation* fdata, const VDXFilterFunctions*) -> int{
            Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
            inst_data->renderer->render(reinterpret_cast<unsigned char*>(fdata->src.data), fdata->src.pitch, fdata->src.mFrameTimestampStart / 10, fdata->src.mFrameTimestampEnd / 10);
            // Success
            return 0;
        },
        // paramProc (Filter video format)
        [](VDXFilterActivation*, const VDXFilterFunctions*) -> long{
            // Use default format (RGB, bottom-up)
            return 0;
        },
        // configProc (Filter configuration)
        [](VDXFilterActivation* fdata, const VDXFilterFunctions*, VDXHWND wnd) -> int{
            // Show modal dialog for filter configuration
            return DialogBoxParamW(reinterpret_cast<HINSTANCE>(module), MAKEINTRESOURCEW(VDUB_DIALOG), reinterpret_cast<HWND>(wnd), config_message_handler, reinterpret_cast<LPARAM>(fdata->filter_data));
        },
        // stringProc (Filter description)
        [](const VDXFilterActivation* fdata, const VDXFilterFunctions*, char* buf) -> void{
            fill_description(reinterpret_cast<Userdata*>(fdata->filter_data), buf);
        },
        // startProc (Filter start running)
        [](VDXFilterActivation* fdata, const VDXFilterFunctions* ffuncs) -> int{
            Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
            // All video informations available?
            if(fdata->pfsi == nullptr)
                ffuncs->Except("Video informations are missing!");
            // Free previous renderer (in case of buggy twice start)
            delete inst_data->renderer; inst_data->renderer = nullptr;
            // Allocate renderer
            try{
                inst_data->renderer = new Renderer(fdata->src.w, fdata->src.h, Renderer::Colorspace::BGRX, *inst_data->script, inst_data->warnings);
            }catch(std::string err){
                ffuncs->Except(err.c_str());
                return 1;
            }
            // Success
            return 0;
        },
        // endProc (Filter end running)
        [](VDXFilterActivation* fdata, const VDXFilterFunctions*) -> int{
            Userdata* inst_data = reinterpret_cast<Userdata*>(fdata->filter_data);
            delete inst_data->renderer; inst_data->renderer = nullptr;
            // Success
            return 0;
        },

        nullptr,	// script_obj
        nullptr,	// fssProc

        // stringProc2 (Filter description *newer versions*)
        [](const VDXFilterActivation* fdata, const VDXFilterFunctions*, char* buf, int maxlen) -> void{
            fill_description(reinterpret_cast<Userdata*>(fdata->filter_data), buf, maxlen);
        },
        nullptr,	// serializeProc
        nullptr,	// deserializeProc
        nullptr,	// copyProc

        nullptr,	// prefetchProc

        nullptr,	// copyProc2
        nullptr,	// prefetchProc2
        nullptr     // eventProc
    };
}

// VirtualDub plugin interface - register
VDXFilterDefinition *registered_filter_definition;
extern "C" __declspec(dllexport) int VirtualdubFilterModuleInit2(struct VDXFilterModule* fmodule, const VDXFilterFunctions* ffuncs, int& vdfd_ver, int& vdfd_compat){
	// Create register definition
	registered_filter_definition = ffuncs->addFilter(fmodule, &VDub::filter_definition, sizeof(VDXFilterDefinition));
	// Version & compatibility definition
	vdfd_ver = VIRTUALDUB_FILTERDEF_VERSION;
	vdfd_compat = VIRTUALDUB_FILTERDEF_COMPATIBLE_COPYCTOR;
	// Success
	return 0;
}

// VirtualDub plugin interface - unregister
extern "C" __declspec(dllexport) void VirtualdubFilterModuleDeinit(struct VDXFilterModule*, const VDXFilterFunctions *ffuncs){
	// Remove register definition
	ffuncs->removeFilter(registered_filter_definition);
}
