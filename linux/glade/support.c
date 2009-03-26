#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "support.h"
#include "interface.h"
#include "ym2612.h"
#include "gens.h"
#include "psg.h"
#include "pcm.h"
#include "pwm.h"
#include "g_ddraw.h"
#include "g_dsound.h"
#include "g_input.h"
#include "g_main.h"
#include "cd_sys.h"
#include "mem_m68k.h"
#include "vdp_rend.h"
#include "vdp_io.h"
#include "ggenie.h"
#include "glade/callbacks.h"
#include "save.h"
#include "scrshot.h"

GtkListStore* listmodel = NULL;

GtkWidget*
lookup_widget                          (GtkWidget       *widget,
                                        const gchar     *widget_name)
{
  GtkWidget *parent, *found_widget;

  for (;;)
    {
      if (GTK_IS_MENU (widget))
        parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
      else
        parent = widget->parent;
      if (!parent)
        parent = g_object_get_data (G_OBJECT (widget), "GladeParentKey");
      if (parent == NULL)
        break;
      widget = parent;
    }

  found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget),
                                                 widget_name);
  if (!found_widget)
    g_warning ("Widget not found: %s", widget_name);
  return found_widget;
}

static GList *pixmaps_directories = NULL;

/* Use this function to set the directory containing installed pixmaps. */
void
add_pixmap_directory                   (const gchar     *directory)
{
  pixmaps_directories = g_list_prepend (pixmaps_directories,
                                        g_strdup (directory));
}

/* This is an internally used function to find pixmap files. */
static gchar*
find_pixmap_file                       (const gchar     *filename)
{
  GList *elem;

  /* We step through each of the pixmaps directory to find it. */
  elem = pixmaps_directories;
  while (elem)
    {
      gchar *pathname = g_strdup_printf ("%s%s%s", (gchar*)elem->data,
                                         G_DIR_SEPARATOR_S, filename);
      if (g_file_test (pathname, G_FILE_TEST_EXISTS))
        return pathname;
      g_free (pathname);
      elem = elem->next;
    }
  return NULL;
}

/* This is an internally used function to create pixmaps. */
GtkWidget*
create_pixmap                          (GtkWidget       *widget,
                                        const gchar     *filename)
{
  gchar *pathname = NULL;
  GtkWidget *pixmap;

  if (!filename || !filename[0])
      return gtk_image_new ();

  pathname = find_pixmap_file (filename);

  if (!pathname)
    {
      g_warning ("Couldn't find pixmap file: %s", filename);
      return gtk_image_new ();
    }

  pixmap = gtk_image_new_from_file (pathname);
  g_free (pathname);
  return pixmap;
}

/* This is an internally used function to create pixmaps. */
GdkPixbuf*
create_pixbuf                          (const gchar     *filename)
{
  gchar *pathname = NULL;
  GdkPixbuf *pixbuf;
  GError *error = NULL;

  if (!filename || !filename[0])
      return NULL;

  pathname = find_pixmap_file (filename);

  if (!pathname)
    {
      g_warning ("Couldn't find pixmap file: %s", filename);
      return NULL;
    }

  pixbuf = gdk_pixbuf_new_from_file (pathname, &error);
  if (!pixbuf)
    {
      fprintf (stderr, "Failed to load pixbuf file: %s: %s\n",
               pathname, error->message);
      g_error_free (error);
    }
  g_free (pathname);
  return pixbuf;
}

/* This is used to set ATK action descriptions. */
void
glade_set_atk_action_description       (AtkAction       *action,
                                        const gchar     *action_name,
                                        const gchar     *description)
{
  gint n_actions, i;

  n_actions = atk_action_get_n_actions (action);
  for (i = 0; i < n_actions; i++)
    {
      if (!strcmp (atk_action_get_name (action, i), action_name))
        atk_action_set_description (action, i, description);
    }
}

void addCode(GtkWidget* treeview, const char* name, const char* code, int selected)
{
	GtkTreeIter iter;
	gtk_list_store_append(listmodel, &iter);
	gtk_list_store_set(GTK_LIST_STORE(listmodel),&iter,
						0, name,
						1, code,
						-1);
	if (selected && treeview)
	{
		GtkTreeSelection* select;
		select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
		gtk_tree_selection_select_iter(select, &iter);
	}
}


void open_game_genie()
{
	GtkWidget* game_genie;
	GtkWidget* treeview;
	GtkCellRenderer* renderer;
	GtkTreeViewColumn* column, *column2;
	GtkTreeSelection* select;
	int i;

	game_genie = create_game_genie();
	treeview = lookup_widget(game_genie, "ggListCode");
	select = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	gtk_tree_selection_set_mode(select,GTK_SELECTION_MULTIPLE);
		
	if (listmodel)
	{
		gtk_list_store_clear(listmodel);
	} else {
		listmodel = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(listmodel));
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes ("Name", renderer, "text", 0,NULL);
	column2 = gtk_tree_view_column_new_with_attributes ("Code", renderer, "text", 1,NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column2);
	
	for(i = 0; i < 256; i++)
	{
		if (Liste_GG[i].code[0] != 0)
		{
			addCode(treeview, Liste_GG[i].name, Liste_GG[i].code,Liste_GG[i].active);

			if ((Liste_GG[i].restore != 0xFFFFFFFF) && (Liste_GG[i].addr < Rom_Size) && (Genesis_Started))
			{
				Rom_Data[Liste_GG[i].addr] = (unsigned char)(Liste_GG[i].restore & 0xFF);
				Rom_Data[Liste_GG[i].addr + 1] = (unsigned char)((Liste_GG[i].restore & 0xFF00) >> 8);
			}
		}
	}

	gtk_widget_show_all(game_genie);
}



GtkWidget* create_file_selector_private(const char* filter, int onlyDir)
{
	GtkWidget* file_sel;
	file_sel = create_file_selector();
	/*if (onlyDir)
		gtk_widget_hide(GTK_FILE_SELECTION(file_sel)->history_pulldown);*/
		
	if (filter)
		gtk_file_selection_complete(GTK_FILE_SELECTION(file_sel), filter);
	return file_sel;
}

void sync_gens_ui()
{
	GtkWidget *vsync, *stretch, *sprite_limit, *perfect_synchro;
	GtkWidget *play_movie;
	GtkWidget *sram_size_0;
	GtkWidget *sram_size[4];

	GtkWidget *enable_sound, *rate_11, *rate_22, *rate_44, *stereo;
	GtkWidget *ym2612, *ym2612imp, *dac, *dacimp,*psg, *psgimp,*z80, *pcm, *pwm, *cdda;

	GtkWidget *rst_main_68k, *rst_sub_68k, *rst_68k, *rst_msh2, *rst_ssh2;

	GtkWidget *render[10];
	GtkWidget *state[10];
	GtkWidget *frame_skip_auto;
	GtkWidget *frame_skip[9];

	GtkWidget* rom_menu;
	GtkWidget* rom_item;
	int i;


	rst_main_68k = lookup_widget(gens_window, "reset_main_68000");
	rst_sub_68k = lookup_widget(gens_window, "reset_sub_68000");
	rst_68k = lookup_widget(gens_window, "reset_68000");
	rst_msh2 = lookup_widget(gens_window, "reset_main_sh2");
	rst_ssh2 = lookup_widget(gens_window, "reset_sub_sh2");
	
	vsync = lookup_widget(gens_window, "vsync");
	stretch = lookup_widget(gens_window, "stretch");
	sprite_limit = lookup_widget(gens_window, "sprite_limit");
	perfect_synchro = lookup_widget(gens_window, "perfect_synchro");
	play_movie = lookup_widget(gens_window, "play_movie");

	sram_size_0 = lookup_widget(gens_window, "none");
	sram_size[0] = lookup_widget(gens_window, "_8_kb");
	sram_size[1] = lookup_widget(gens_window, "_16_kb");
	sram_size[2] = lookup_widget(gens_window, "_32_kb");
	sram_size[3] = lookup_widget(gens_window, "_64_kb");
		
	enable_sound = lookup_widget(gens_window, "enable_sound");
	rate_11 = lookup_widget(gens_window, "rate_11025");
	rate_22 = lookup_widget(gens_window, "rate_22050");
	rate_44 = lookup_widget(gens_window, "rate_44100");
	ym2612 = lookup_widget(gens_window, "ym2612");
	ym2612imp = lookup_widget(gens_window, "ym2612_improved");
	dac = lookup_widget(gens_window, "dac");
	dacimp = lookup_widget(gens_window, "dac_improved");
	psg = lookup_widget(gens_window, "psg");
	psgimp = lookup_widget(gens_window, "psg_improved");
	z80 = lookup_widget(gens_window, "z80");
	pcm = lookup_widget(gens_window, "pcm");
	pwm = lookup_widget(gens_window, "pwm");
	cdda = lookup_widget(gens_window, "cdda");
	stereo = lookup_widget(gens_window, "stereo");
	
	render[0] = lookup_widget(gens_window, "normal");
	render[1] = lookup_widget(gens_window, "_double");
	render[2] = lookup_widget(gens_window, "interpolated");
	render[3] = lookup_widget(gens_window, "scanline");
	render[4] = lookup_widget(gens_window, "_50_scanline");
	render[5] = lookup_widget(gens_window, "_25_scanline");
	render[6] = lookup_widget(gens_window, "interpolated_scanline");
	render[7] = lookup_widget(gens_window, "interpolated_50_scanline");
	render[8] = lookup_widget(gens_window, "interpolated_25_scanline");
	render[9] = lookup_widget(gens_window, "_2xsai_kreed");
	
	state[0] = lookup_widget(gens_window, "change_state_slot0");
	state[1] = lookup_widget(gens_window, "change_state_slot1");
	state[2] = lookup_widget(gens_window, "change_state_slot2");
	state[3] = lookup_widget(gens_window, "change_state_slot3");
	state[4] = lookup_widget(gens_window, "change_state_slot4");
	state[5] = lookup_widget(gens_window, "change_state_slot5");
	state[6] = lookup_widget(gens_window, "change_state_slot6");
	state[7] = lookup_widget(gens_window, "change_state_slot7");
	state[8] = lookup_widget(gens_window, "change_state_slot8");
	state[9] = lookup_widget(gens_window, "change_state_slot9");
	
	frame_skip_auto = lookup_widget(gens_window, "frame_auto_skip");
	frame_skip[0] = lookup_widget(gens_window, "frame_0_skip");
	frame_skip[1] = lookup_widget(gens_window, "frame_1_skip");
	frame_skip[2] = lookup_widget(gens_window, "frame_2_skip");
	frame_skip[3] = lookup_widget(gens_window, "frame_3_skip");
	frame_skip[4] = lookup_widget(gens_window, "frame_4_skip");
	frame_skip[5] = lookup_widget(gens_window, "frame_5_skip");
	frame_skip[6] = lookup_widget(gens_window, "frame_6_skip");
	frame_skip[7] = lookup_widget(gens_window, "frame_7_skip");
	frame_skip[8] = lookup_widget(gens_window, "frame_8_skip");

	gtk_widget_hide(rst_main_68k);
	gtk_widget_hide(rst_sub_68k);
	gtk_widget_hide(rst_68k);
	gtk_widget_hide(rst_msh2);
	gtk_widget_hide(rst_ssh2);

	if (SegaCD_Started)
	{
		gtk_widget_show(rst_main_68k);
		gtk_widget_show(rst_sub_68k);
	}
	else if (_32X_Started)
	{
		gtk_widget_show(rst_68k);
		gtk_widget_show(rst_msh2);
		gtk_widget_show(rst_ssh2);
	}
	else
	{
		gtk_widget_show(rst_68k);
	}

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(vsync), W_VSync);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(stretch), Stretch);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sprite_limit), Sprite_Over);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(perfect_synchro), SegaCD_Accurate);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(enable_sound), Sound_Enable);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(stereo), Sound_Stereo);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(z80), Z80_State);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ym2612), YM2612_Enable);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ym2612imp), YM2612_Improv);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(dac), DAC_Enable);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(dacimp), DAC_Improv);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(psg), PSG_Enable);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(psgimp), PSG_Improv);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pcm), PCM_Enable);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(pwm), PWM_Enable);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(cdda), CDDA_Enable);

	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(state[Current_State]), 1);
	if (-1 == Frame_Skip)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(frame_skip_auto), 1);
	else
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(frame_skip[Frame_Skip]), 1);
	
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(render[Render_W]), 1);
	
	switch(Sound_Rate)
	{
		case 11025:
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(rate_11), 1);
			break;
		case 22050:
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(rate_22), 1);
			break;
		case 44100:
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(rate_44), 1);
			break;
	}

	if (BRAM_Ex_State & 0x100)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sram_size[BRAM_Ex_Size]), 1);
	else
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(sram_size_0), 1);
	
	
	rom_item = lookup_widget(gens_window, "rom_history");
	gtk_menu_item_remove_submenu(GTK_MENU_ITEM(rom_item));
	rom_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(rom_item), rom_menu);
	
	for(i = 0; i < 9; i++)
	{
		if (strcmp(Recent_Rom[i], ""))
		{
			char tmp[1024];
			GtkWidget* item;
		
			switch (Detect_Format(Recent_Rom[i]) >> 1)			// do not exist anymore
			{
				default:
					strcpy(tmp, "[---]\t- ");
					break;

				case 1:
					strcpy(tmp, "[MD]\t- ");
					break;

				case 2:
					strcpy(tmp, "[32X]\t- ");
					break;

				case 3:
					strcpy(tmp, "[SCD]\t- ");
					break;

				case 4:
					strcpy(tmp, "[SCDX]\t- ");
					break;
			}

			Get_Name_From_Path(Recent_Rom[i], Str_Tmp);
			strcat(tmp, Str_Tmp);
			
			item = gtk_menu_item_new_with_label(tmp);
			gtk_widget_show(item);
			gtk_container_add(GTK_CONTAINER(rom_menu), item);
			g_signal_connect((gpointer) item, "activate", G_CALLBACK(on_rom_history_activate), GINT_TO_POINTER(i));
		}
		else break;
	}
	
}

static void
Setting_Keys_Proxy(GtkWidget* button, gpointer data)
{
	GtkWidget* control_window;
	GtkWidget* pad;
	int player;
	int type;
	control_window = lookup_widget(button, "controllers_settings");
	player = GPOINTER_TO_INT(data);
	
	switch(player)
	{
		case 0: pad = lookup_widget(control_window, "padp1"); break;
		case 2: pad = lookup_widget(control_window, "padp1b"); break;
		case 3: pad = lookup_widget(control_window, "padp1c"); break;
		case 4: pad = lookup_widget(control_window, "padp1d"); break;
		case 1: pad = lookup_widget(control_window, "padp2"); break;
		case 5: pad = lookup_widget(control_window, "padp2b"); break;
		case 6: pad = lookup_widget(control_window, "padp2c"); break;
		case 7: pad = lookup_widget(control_window, "padp2d"); break;
	}

	type = gtk_option_menu_get_history(GTK_OPTION_MENU(pad));
	
	Setting_Keys(control_window, player, type);
}


void open_joypads()
{
	GtkWidget* ctrlset;
	GtkWidget* redef1, *redef1b, *redef1c,*redef1d;
	GtkWidget* redef2, *redef2b, *redef2c,*redef2d;

	//if (Check_If_Kaillera_Running()) return 0;
	End_Input();
	Init_Input(0, 0);
	ctrlset = create_controllers_settings();
	redef1 = lookup_widget(ctrlset, "buttonRedef1");
	redef1b = lookup_widget(ctrlset, "buttonRedef1B");
	redef1c = lookup_widget(ctrlset, "buttonRedef1C");
	redef1d = lookup_widget(ctrlset, "buttonRedef1D");

	redef2 = lookup_widget(ctrlset, "buttonRedef2");
	redef2b = lookup_widget(ctrlset, "buttonRedef2B");
	redef2c = lookup_widget(ctrlset, "buttonRedef2C");
	redef2d = lookup_widget(ctrlset, "buttonRedef2D");

	g_signal_connect(GTK_OBJECT(redef1), "clicked", G_CALLBACK(Setting_Keys_Proxy), GINT_TO_POINTER(0));
	g_signal_connect(GTK_OBJECT(redef1b), "clicked", G_CALLBACK(Setting_Keys_Proxy), GINT_TO_POINTER(2));
	g_signal_connect(GTK_OBJECT(redef1c), "clicked", G_CALLBACK(Setting_Keys_Proxy), GINT_TO_POINTER(3));
	g_signal_connect(GTK_OBJECT(redef1d), "clicked", G_CALLBACK(Setting_Keys_Proxy), GINT_TO_POINTER(4));
	
	g_signal_connect(GTK_OBJECT(redef2), "clicked", G_CALLBACK(Setting_Keys_Proxy), GINT_TO_POINTER(1));
	g_signal_connect(GTK_OBJECT(redef2b), "clicked", G_CALLBACK(Setting_Keys_Proxy), GINT_TO_POINTER(5));
	g_signal_connect(GTK_OBJECT(redef2c), "clicked", G_CALLBACK(Setting_Keys_Proxy), GINT_TO_POINTER(6));
	g_signal_connect(GTK_OBJECT(redef2d), "clicked", G_CALLBACK(Setting_Keys_Proxy), GINT_TO_POINTER(7));
	
	gtk_widget_show_all(ctrlset);
	Init_Input(0, HWnd);
}

static void
on_change_dir(GtkWidget* button, gpointer data)
{
	GtkWidget* file_sel;
	int res;
	gchar* tmp;
	gchar* filename;

	file_sel = create_file_selector_private(NULL, 1);
	res = gtk_dialog_run(GTK_DIALOG(file_sel));

	switch(res)
	{
		case GTK_RESPONSE_OK:
			tmp = (gchar*)gtk_file_selection_get_filename(GTK_FILE_SELECTION(file_sel));
			filename = malloc(strlen(tmp)+1);
			strcpy(filename,tmp);
			gtk_entry_set_text(GTK_ENTRY(data), filename);
			break;
		case GTK_RESPONSE_CANCEL:
			break;
	}
	
	gtk_widget_destroy(file_sel);
}

void open_dir_config()
{

	GtkWidget* dir;
	GtkWidget* states, *sram, *bram, *wav, *gym, *screenshot, *pat, *ips;
	GtkWidget* btnstates, *btnsram, *btnbram, *btnwav, *btngym, *btnscreenshot, *btnpat, *btnips;

	dir = create_directories_configuration();

	btnstates = lookup_widget(dir, "buttonStatesDir");
	btnsram = lookup_widget(dir, "buttonSramDir");
	btnbram = lookup_widget(dir, "buttonBramDir");
	btnwav = lookup_widget(dir, "buttonWavDir");
	btngym = lookup_widget(dir, "buttonGymDir");
	btnscreenshot = lookup_widget(dir, "buttonScreenshotDir");
	btnpat = lookup_widget(dir, "buttonPatDir");
	btnips = lookup_widget(dir, "buttonIpsDir");

	states = lookup_widget(dir, "statesDir");
	sram = lookup_widget(dir, "sramDir");
	bram = lookup_widget(dir, "bramDir");
	wav = lookup_widget(dir, "wavDir");
	gym = lookup_widget(dir, "gymDir");
	screenshot = lookup_widget(dir, "screenshotDir");
	pat = lookup_widget(dir, "patDir");
	ips = lookup_widget(dir, "ipsDir");
	
	gtk_entry_set_text(GTK_ENTRY(states), State_Dir);
	gtk_entry_set_text(GTK_ENTRY(sram), SRAM_Dir);
	gtk_entry_set_text(GTK_ENTRY(bram), BRAM_Dir);
	gtk_entry_set_text(GTK_ENTRY(wav), Dump_Dir);
	gtk_entry_set_text(GTK_ENTRY(gym), Dump_GYM_Dir);
	gtk_entry_set_text(GTK_ENTRY(screenshot), ScrShot_Dir);
	gtk_entry_set_text(GTK_ENTRY(pat), Patch_Dir);
	gtk_entry_set_text(GTK_ENTRY(ips), IPS_Dir);
	
	g_signal_connect(GTK_OBJECT(btnstates), "clicked", G_CALLBACK(on_change_dir), states);
	g_signal_connect(GTK_OBJECT(btnsram), "clicked", G_CALLBACK(on_change_dir), sram);
	g_signal_connect(GTK_OBJECT(btnbram), "clicked", G_CALLBACK(on_change_dir), bram);
	g_signal_connect(GTK_OBJECT(btnwav), "clicked", G_CALLBACK(on_change_dir), wav);	
	g_signal_connect(GTK_OBJECT(btngym), "clicked", G_CALLBACK(on_change_dir), gym);
	g_signal_connect(GTK_OBJECT(btnscreenshot), "clicked", G_CALLBACK(on_change_dir), screenshot);
	g_signal_connect(GTK_OBJECT(btnpat), "clicked", G_CALLBACK(on_change_dir), pat);
	g_signal_connect(GTK_OBJECT(btnips), "clicked", G_CALLBACK(on_change_dir), ips);	

	gtk_widget_show_all(dir);
}

void open_bios_cfg()
{
	GtkWidget* dir;
	GtkWidget* genesis, *m68000, *msh2, *ssh2, *usabios, *eurbios, *japbios, *cgoffline, *manual;
	GtkWidget* btngenesis, *btnm68000, *btnmsh2, *btnssh2, *btnusabios, *btneurbios, *btnjapbios, *btncgoffline, *btnmanual;

	dir = create_bios_files();

	btngenesis = lookup_widget(dir, "buttonGenesisBios");
	btnm68000 = lookup_widget(dir, "buttonM68000");
	btnmsh2 = lookup_widget(dir, "buttonMSH2");
	btnssh2 = lookup_widget(dir, "buttonSSH2");
	btnusabios = lookup_widget(dir, "buttonUSABios");
	btneurbios = lookup_widget(dir, "buttonEURBios");
	btnjapbios = lookup_widget(dir, "buttonJAPBios");
	btncgoffline = lookup_widget(dir, "buttonCGOffline");
	btnmanual = lookup_widget(dir, "buttonManual");

	genesis = lookup_widget(dir, "genesisBios");
	m68000 = lookup_widget(dir, "M68000");
	msh2 = lookup_widget(dir, "MSH2");
	ssh2= lookup_widget(dir, "SSH2");
	usabios = lookup_widget(dir, "USAbios");
	eurbios = lookup_widget(dir, "EURbios");
	japbios = lookup_widget(dir, "JAPbios");
	cgoffline = lookup_widget(dir, "CGOffline");
	manual = lookup_widget(dir, "manual");
	
	gtk_entry_set_text(GTK_ENTRY(genesis), Genesis_Bios);
	gtk_entry_set_text(GTK_ENTRY(m68000), _32X_Genesis_Bios);
	gtk_entry_set_text(GTK_ENTRY(msh2), _32X_Master_Bios);
	gtk_entry_set_text(GTK_ENTRY(ssh2), _32X_Slave_Bios);
	gtk_entry_set_text(GTK_ENTRY(usabios), US_CD_Bios);
	gtk_entry_set_text(GTK_ENTRY(eurbios), EU_CD_Bios);
	gtk_entry_set_text(GTK_ENTRY(japbios), JA_CD_Bios);
	gtk_entry_set_text(GTK_ENTRY(cgoffline), CGOffline_Path);
	gtk_entry_set_text(GTK_ENTRY(manual), Manual_Path);
	
	g_signal_connect(GTK_OBJECT(btngenesis), "clicked", G_CALLBACK(on_change_dir), genesis);
	g_signal_connect(GTK_OBJECT(btnm68000), "clicked", G_CALLBACK(on_change_dir), m68000);
	g_signal_connect(GTK_OBJECT(btnmsh2), "clicked", G_CALLBACK(on_change_dir), msh2);
	g_signal_connect(GTK_OBJECT(btnssh2), "clicked", G_CALLBACK(on_change_dir), ssh2);	
	g_signal_connect(GTK_OBJECT(btnusabios), "clicked", G_CALLBACK(on_change_dir), usabios);
	g_signal_connect(GTK_OBJECT(btneurbios), "clicked", G_CALLBACK(on_change_dir), eurbios);
	g_signal_connect(GTK_OBJECT(btnjapbios), "clicked", G_CALLBACK(on_change_dir), japbios);
	g_signal_connect(GTK_OBJECT(btncgoffline), "clicked", G_CALLBACK(on_change_dir), cgoffline);	
	g_signal_connect(GTK_OBJECT(btnmanual), "clicked", G_CALLBACK(on_change_dir), manual);	

	gtk_widget_show_all(dir);
}
