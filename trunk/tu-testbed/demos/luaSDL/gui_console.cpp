// gui_console.cpp	-- by Thatcher Ulrich <tu@tulrich.com> Oct 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Simple Lua GUI console.  Type Lua code into a text widget, and see
// the output in another text widget.


#include <stdio.h>
#include <stdlib.h>

#include <engine/config.h>

#include "gtk.h"
#include "gdk/gdk.h"
#include "gdk/gdkkeysyms.h"

#include "gui_console.h"


GtkWidget*	button_new_with_accelerator(const char* label, GtkAccelGroup* accel_group)
// Convenience function; creates a button with an underlined ALT-key
// accelerator.  Looks for the "_" character in the given label to
// place the underline and pick the accelerator key.
{
	GtkWidget*	button = gtk_button_new();
	GtkWidget*	label_widget = gtk_accel_label_new(label);
	gtk_widget_show(label_widget);
	gtk_container_add(GTK_CONTAINER(button), label_widget);
	guint accelerator_key = gtk_label_parse_uline(GTK_LABEL(label_widget), label);

	if (accelerator_key != GDK_VoidSymbol) {
		// Map Alt-X to execute_entry
		gtk_widget_add_accelerator (button,
									"clicked",
									accel_group,
									accelerator_key,
									GDK_MOD1_MASK,	// GDK_CONTROL_MASK, GDK_SHIFT_MASK
									(GtkAccelFlags) GTK_ACCEL_LOCKED);
	}

	return button;
}


static GtkWidget*	console_input;
static GtkWidget*	output;


static void	execute_entry(GtkWidget* w, GtkText* text)
// Handler for the "execute" button.
//
// Run lua_dostring() on the current contents of the text entry
// buffer.
{
//	int	length = gtk_text_get_length(text);

	gchar*	entry = gtk_editable_get_chars(&text->editable, 0, -1);

	if (lua_dostring(config::L, entry) != 0) {
		// error executing the text.  Leave the entry window as-is, so
		// the user can fix the error and retry.
		
		// TODO: parse the error output, and highlight & jump to the
		// error line if possible.

	} else {
		// Successful execution.

		// TODO: Move the text to a history buffer.

		// Clear the entry buffer.
		gtk_text_set_point(text, 0);
		gtk_text_forward_delete(text, gtk_text_get_length(text));
	}

	g_free(entry);
}


void	close_console(GtkWidget* w, GtkText* text)
// Close the console window, and exit gui_console().
{
	// Close our console window.
	gtk_widget_destroy(w);

	// Exit gtk.
	gtk_main_quit();
}


static void	create_console()
// Create a window to serve as the Lua programming console.  Includes
// entry and output windows.
{
	int i, j;

	static GtkWidget *window = NULL;
	GtkWidget *box1;
	GtkWidget *box2;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *check;
	GtkWidget *separator;
	GtkWidget *scrolled_window;
	GdkFont *font;
	GtkAccelGroup*	accel_group;

	FILE *infile;

	if (!window)
    {
		window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
		gtk_widget_set_name (window, "luatoy Lua programming console");
		gtk_widget_set_usize (window, 500, 500);
		gtk_window_set_policy (GTK_WINDOW(window), TRUE, TRUE, FALSE);

		gtk_signal_connect (GTK_OBJECT (window), "destroy",
							GTK_SIGNAL_FUNC(gtk_widget_destroyed),
							&window);

		accel_group = gtk_accel_group_new ();
		gtk_accel_group_attach (accel_group, GTK_OBJECT (window));

		gtk_window_set_title (GTK_WINDOW (window), "luatoy Lua programming console");
		gtk_container_set_border_width (GTK_CONTAINER (window), 0);


		box1 = gtk_vbox_new (FALSE, 0);
		gtk_container_add (GTK_CONTAINER (window), box1);
		gtk_widget_show (box1);


		box2 = gtk_vbox_new (FALSE, 10);
		gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
		gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);
		gtk_widget_show (box2);

		//
		// output text widget.
		//
		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_box_pack_start (GTK_BOX (box2), scrolled_window, TRUE, TRUE, 0);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
										GTK_POLICY_NEVER,
										GTK_POLICY_ALWAYS);
		gtk_widget_show (scrolled_window);

		output = gtk_text_new(NULL, NULL);
		gtk_text_set_editable(GTK_TEXT(output), FALSE);
		gtk_container_add(GTK_CONTAINER(scrolled_window), output);
		gtk_widget_show(output);

		//
		// entry text widget.
		//
		if (0) {
			GtkWidget*	label_widget = gtk_label_new("code entry:");
			gtk_widget_show(label_widget);
			gtk_container_add(GTK_CONTAINER(box2), label_widget);
			gtk_box_pack_start(GTK_BOX(box2), label_widget, FALSE, FALSE, 0);
		}
		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_box_pack_start (GTK_BOX (box2), scrolled_window, TRUE, TRUE, 0);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
										GTK_POLICY_NEVER,
										GTK_POLICY_ALWAYS);
		gtk_widget_show (scrolled_window);
		console_input = gtk_text_new (NULL, NULL);
		gtk_text_set_editable (GTK_TEXT (console_input), TRUE);
		gtk_container_add (GTK_CONTAINER (scrolled_window), console_input);
		gtk_widget_grab_focus (console_input);
		gtk_widget_show (console_input);
		gtk_container_set_focus_child(GTK_CONTAINER(scrolled_window), console_input);	// will this initialize the focus into the entry window?

//		gtk_text_freeze (GTK_TEXT (console_input));
//
//		font = gdk_font_load ("-adobe-courier-medium-r-normal--*-120-*-*-*-*-*-*");
//		/* The Text widget will reference count the font, so we
//		 * unreference it here
//		 */
//		gdk_font_unref (font);
//
//		gtk_text_thaw (GTK_TEXT (console_input));


		hbox = gtk_hbutton_box_new ();
		gtk_box_pack_start (GTK_BOX (box2), hbox, FALSE, FALSE, 0);
		gtk_widget_show (hbox);

		separator = gtk_hseparator_new ();
		gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);
		gtk_widget_show (separator);


		box2 = gtk_vbox_new (FALSE, 10);
		gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
		gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, TRUE, 0);
		gtk_widget_show (box2);


		button = button_new_with_accelerator("e_xecute", accel_group);
		gtk_signal_connect (GTK_OBJECT (button), "clicked",
							GTK_SIGNAL_FUNC(execute_entry),
							GTK_TEXT (console_input));
		gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
		gtk_widget_show (button);


		button = button_new_with_accelerator("_close", accel_group);
		gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
								   GTK_SIGNAL_FUNC(close_console),
								   GTK_OBJECT (window));
		gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
		GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
		gtk_widget_grab_default (button);
		gtk_widget_show (button);

		gtk_window_activate_focus(GTK_WINDOW(window));
    }

	if (!GTK_WIDGET_VISIBLE (window))
		gtk_widget_show (window);
	else
		gtk_widget_destroy (window);
}


static void	insert_output(const char* str)
// Inserts the given string into the output window.
{
	gtk_text_freeze(GTK_TEXT(output));
	gtk_text_set_point(GTK_TEXT(output), gtk_text_get_length(GTK_TEXT(output)));
	gtk_text_insert(GTK_TEXT(output), NULL, NULL, NULL, str, -1);
	gtk_text_thaw(GTK_TEXT(output));
}


static int	wrap_lua_alert(lua_State* L)
// Send error output from Lua to the GUI output widget.
{
	insert_output(lua_tostring(L, -1));
	return 0;	// return no results.
}


static int	wrap_lua_print(lua_State* L)
// Replacement for the Lua 'print' function, which uses the GUI output
// window instead of stdout.
{
	int	argc = lua_gettop(L);

	for (int i=1; i <= argc; i++) {
		const char*	str = lua_tostring(L, i);
		
		if (str) {
			insert_output(str);
		} else {
			insert_output("<not a string>");	// @@ TODO: handle tables and udata better.
		}

		// Insert tabs between arguments.
		if (i < argc) {
			insert_output("\t");
		}
	}
	insert_output("\n");
	return 0;	// return no results.
}


void	gui_console()
// Run a simple Lua programming console.  Allow code editing and entry
// into a text widget, and submit the code to Lua when the user
// presses a button.  Collect the output into another text-output
// widet.
{
	// @@ for the moment, just try to get a GTK text widget up on the
	// screen through any viable means.

	gtk_set_locale ();

//	gtk_rc_add_default_file ("testgtkrc");

	int	argc = 0;
	char* argv[] = { "bobo" };

	gtk_init (&argc, (char***) &argv);

	gdk_rgb_init ();

//	/* bindings test
//	 */
//	binding_set = gtk_binding_set_by_class (gtk_type_class (GTK_TYPE_WIDGET));
//	gtk_binding_entry_add_signal (binding_set,
//								  '9', GDK_CONTROL_MASK | GDK_RELEASE_MASK,
//								  "debug_msg",
//								  1,
//								  GTK_TYPE_STRING, "GtkWidgetClass <ctrl><release>9 test");

	// Create the GUI console window.
	create_console();

	// Capture Lua's output with wrappers that will send it to the
	// GUI output window.
	lua_pushcfunction(config::L, wrap_lua_alert);
	lua_setglobal(config::L, "_ALERT");

	lua_pushcfunction(config::L, wrap_lua_print);
	lua_setglobal(config::L, "print");

	// Let the GUI have control.
	gtk_main ();
}

