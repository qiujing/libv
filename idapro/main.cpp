#include <fstream>
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <funcs.hpp>
#include <diskio.hpp>

using namespace std;

int IDAP_init(void)
{
    // Do checks here to ensure your plug-in is being used within
    // an environment it was written for. Return PLUGIN_SKIP if the
    // checks fail, otherwise return PLUGIN_KEEP.
    return PLUGIN_KEEP;
}

void IDAP_term(void)
{
    // Stuff to do when exiting, generally you'd put any sort
    // of clean-up jobs here.
    return;
}
// The plugin can be passed an integer argument from the plugins.cfg
// file. This can be useful when you want the one plug-in to do
// something different depending on the hot-key pressed or menu
// item selected.

void export_function()
{
    char root_filename[260];

    get_input_file_path(root_filename, 260);
    qstrncat(root_filename, ".txt", 260);

    ofstream outfile (root_filename, ofstream::binary);
    for (int f = 0; f < get_func_qty(); f++)
    {
        func_t *curFunc = getn_func(f);
        outfile.write((const char*)(&curFunc->startEA), sizeof(ea_t));
        outfile.write((const char*)(&curFunc->endEA), sizeof(ea_t));
    }
    outfile.close();

    msg("export %s OK!", root_filename);
}

void export_function_name()
{
    char root_filename[260];
    char funcName[260];

    get_input_file_path(root_filename, 260);
    qstrncat(root_filename, "_name.txt", 260);

    ofstream outfile (root_filename, ofstream::binary);
    for (int f = 0; f < get_func_qty(); f++)
    {
        func_t *curFunc = getn_func(f);
        if (curFunc->endEA - curFunc->startEA < 6)
        {
            continue;
        }

        int ins_count = 0;
        int cur_EA = curFunc->startEA;

        while (cur_EA < curFunc->endEA && ins_count < 6)
        {
            int len = ua_ana0(cur_EA);
			if (len==0) break;
            ins_count++;
            cur_EA += len;
        }
		
		if (ins_count<6) {
			 continue;
		}

        char* p = get_func_name(curFunc->startEA, funcName, 260);
        if (p && (strnicmp(funcName, "sub_", 4) == 0 || strnicmp(funcName, "unknown_libname_", 16) == 0))
        {
            continue;
        }

        outfile << hex << curFunc->startEA << ",\"" << (p ? funcName : "NONAME") << "\"" << endl;
    }
    outfile.close();

    msg("export %s OK!", root_filename);
    return;
}
void IDAP_run(int arg)
{
    // The "meat" of your plug-in
    export_function();
    export_function_name();
    return;
}
// There isn't much use for these yet, but I set them anyway.
char IDAP_comment[] = "LIBV export function plugin";
char IDAP_help[] = "LIBV";
// The name of the plug-in displayed in the Edit->Plugins menu. It can
// be overridden in the user's plugins.cfg file.
char IDAP_name[] = "LIBV Plugin";
// The hot-key the user can use to run your plug-in.
char IDAP_hotkey[] = "Alt-X";
// The all-important exported PLUGIN object
plugin_t PLUGIN =
{
    IDP_INTERFACE_VERSION, // IDA version plug-in is written for
    0, // Flags (see below)
    IDAP_init, // Initialisation function
    IDAP_term, // Clean-up function
    IDAP_run, // Main plug-in body
    IDAP_comment, // Comment ¨C unused
    IDAP_help, // As above ¨C unused
    IDAP_name, // Plug-in name shown in
    // Edit->Plugins menu
    IDAP_hotkey // Hot key to run the plug-in
};