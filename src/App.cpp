//#define DEBUG 1
#define BUILD_QUERYWATCHER 1
#include "App.h"

#ifdef DEBUG
#include <BeDC.h>
#include <Debug.h>
#include <String.h>
#endif

// eiman
#include <Path.h>
#include <stdio.h>

#include <Alert.h>
#include <Directory.h>
#include <fs_attr.h>
#include <File.h>
#include <Dragger.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <NodeMonitor.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Roster.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <map>

// Loads 'attribute' of 'type' from file 'name'. Returns a BBitmap (Callers 
//  responsibility to delete it) on success, NULL on failure. 

BBitmap *GetBitmapFromAttribute(const char *name, const char *attribute, 
	type_code type = 'BBMP') {

	BBitmap 	*bitmap = NULL;
	size_t 		len = 0;
//	status_t 	error;	

	if ((name == NULL) || (attribute == NULL)) return NULL;

	BNode node(name);
	
	if (node.InitCheck() != B_OK) {
		return NULL;
	};
	
	attr_info info;
		
	if (node.GetAttrInfo(attribute, &info) != B_OK) {
		node.Unset();
		return NULL;
	};
		
	char *data = (char *)calloc(info.size, sizeof(char));
	len = (size_t)info.size;
		
	if (node.ReadAttr(attribute, type, 0, data, len) != (int32)len) {
		node.Unset();
		free(data);
	
		return NULL;
	};
	
//	Icon is a square, so it's right / bottom co-ords are the root of the bitmap length
//	Offset is 0
	BRect bound = BRect(0, 0, 0, 0);
	bound.right = sqrt(len) - 1;
	bound.bottom = bound.right;
	
	bitmap = new BBitmap(bound, B_COLOR_8_BIT);
	bitmap->SetBits(data, len, 0, B_COLOR_8_BIT);

//	make sure it's ok
	if(bitmap->InitCheck() != B_OK) {
		free(data);
		delete bitmap;
		return NULL;
	};

	return bitmap;
}


BBitmap *
get_icon( const char * _path )
{
	//printf("get_icon\n");
	
	BBitmap * icon;
	
	icon = GetBitmapFromAttribute( _path, "BEOS:M:STD_ICON" );
	
	if ( icon )
		return icon;
	
	BEntry entry(_path, true);
	BPath bpath(&entry);
	
	const char * path = bpath.Path();
	
	//printf("  get_icon: actual path: %s (%s)\n", path, _path);
	
	BNode node(path);
	BNodeInfo ninfo(&node);
	
	char mimestring[512];
	
	ninfo.GetType(mimestring);
	
	icon = new BBitmap( BRect(0,0, 15,15), B_CMAP8 );
	bool found_icon = false;
	
	BMimeType mime(mimestring);
	char preferred_app[256];
	if ( mime.GetPreferredApp(preferred_app) != B_OK )
	{
		preferred_app[0] = 0;
	}
	
	if ( mime.GetIcon(icon, B_MINI_ICON) == B_OK) 
		found_icon = true;
	
	
	if ( !found_icon )
	{
		//printf("  get_icon: no icon for mime type, checking supertype\n");
			
		BMimeType super_type;
		mime.GetSupertype(&super_type);
			
		// ask for the supertype's icon
		if (super_type.GetIcon(icon, B_MINI_ICON) == B_OK) {
			found_icon = true;
		}
	}
	
	if ( !found_icon && preferred_app[0] )
	{
		//printf("  get_icon: no icon for supertype, checking preferred app: %s\n", preferred_app);
		
		app_info ai;
		
		if ( be_roster->GetAppInfo( preferred_app, &ai ) == B_OK )
		{ // got app info
			BPath app_path( &ai.ref );
			
			BBitmap * i = GetBitmapFromAttribute( app_path.Path(), "BEOS:M:STD_ICON" );
			
			if ( i )
			{
				delete icon;
				icon = i;
				found_icon = true;
			}
		}
	}
	
	if ( !found_icon )
	{
		//printf("  get_icon: reverting to default icon\n");
		BMimeType("application/octet-stream").GetIcon(icon, B_MINI_ICON);
	}
	
	return icon;
}


struct query_load_info
{
	~query_load_info()
	{
		delete [] predicate;
	}
		
	entry_ref 		ref;
	const char* 	predicate;
	BRect 			rect;	
};

App::App()
	: BApplication(APP_SIG)
{
	//Window* win = new Window( BRect(100,100,300,300), "/boot/home/config/DesktopFolder/Test");
	CreatorWindow * win = new CreatorWindow();
	win->Show();
}

App::~App()
{
	
}

void
App::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		default: 
			BApplication::MessageReceived(msg);
	}
}


CreatorWindow::CreatorWindow()
:	BWindow( BRect(50,50,220,70), "Desktop drawer", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BStringView * v = new BStringView( Bounds(), "text", "Drop a folder here" );
	v->SetAlignment( B_ALIGN_CENTER );
	AddChild( v );
}

CreatorWindow::~CreatorWindow()
{
}

bool
CreatorWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
CreatorWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			
			for ( int i=0; msg->FindRef("refs",i,&ref)==B_OK; i++ )
			{
				BPath path(&ref);
				
				BDirectory dir(&ref);
				
				if ( dir.InitCheck() != B_OK )
				{
					BAlert * alert = new BAlert("title", "Not a directory", "Ok");
					alert->Go();
					continue;
				}
				
				Window * win = new Window( BRect(100,100,300,300), path.Path() );
				win->Show();
			}
		}	break;
		
		default:
			BWindow::MessageReceived(msg);
	}
}


Window::Window(BRect rect, const char * path )
	: BWindow( rect, "Replicant holder", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	fBackground = new ReplicantView( Bounds(), path );
	
	AddChild(fBackground);
}

Window::~Window()
{
}

bool 
Window::QuitRequested()
{
	//be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
Window::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		default:
			BWindow::MessageReceived(msg);
	}
}

ReplicantView::ReplicantView(BRect frame, const char * path)
	: BView( frame, "FolderDesktop", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fPath = path;
	
	SetViewColor(218,218,218);
	
	BPath p( path );
	
	fName = p.Leaf();
	
	fExpanded = true;
	
	Init();
	
}

ReplicantView::~ReplicantView()
{
}

status_t 
ReplicantView::Archive(BMessage* msg, bool deep) const
{
	msg->AddString("add_on", APP_SIG);
	msg->AddString("path", fPath.String() );
	msg->AddString("name", fName.String() );
	msg->AddBool("expanded", fExpanded );
	
	return BView::Archive(msg, false);
}

ReplicantView::ReplicantView(BMessage* msg)
	: BView(msg)
{
	if ( msg->FindString("path") )
	{
		fPath = msg->FindString("path");
	}
	
	if ( msg->FindString("name") )
	{
		fName = msg->FindString("name");
	}
	
	msg->FindBool("expanded", &fExpanded );
	
	Init();
}

void ReplicantView::Init()
{
	fBorderColor = (rgb_color){0,0,0};
	
	SetViewColor(255,255,255);
	
	printf("ReplicantView::Init()\n");
	
	BDirectory dir( fPath.String() );
	
	dir.GetNodeRef( &fNode );
	
	entry_ref ref;
	
	float curry = 16;
	float maxwidth = StringWidth( fName.String() ) + 20;
	
	while ( dir.GetNextRef(&ref) == B_OK )
	{
		BEntry entry(&ref);
		
		BPath path;
		
		entry.GetPath(&path);
		
		printf("Path: %s\n", path.Path());
		
		FileView * f = new FileView(ref);
		AddChild(f);
		f->MoveTo(2,curry);
		
		if ( f->Bounds().Width() > maxwidth )
		{
			maxwidth = f->Bounds().Width();
		}
		
		curry = f->Frame().bottom+2;
	}
	
	ResizeTo( maxwidth + 4, curry );
	
	printf("Done fetching files\n");

	fExpandRect = BRect(0,0,8,8);
	fExpandRect.OffsetTo( Bounds().right-11, 3 );
	
	BDragger *dragger;
	dragger = new BDragger(BRect(0,0,7,7), this, 0);
	//dragger->MoveTo( Bounds().right-8, Bounds().bottom-8 );
	AddChild(dragger);
	
	fMouseDown = false;
	
	UpdateSize();
}

void
ReplicantView::Draw( BRect )
{
	SetHighColor( ViewColor() );
	
	FillRect( Bounds() );
	
	// border
	if ( !fExpanded )
		SetHighColor( fBorderColor );
	else
		SetHighColor(0,0,0);

	StrokeRect( Bounds() );
	StrokeLine( BPoint(0,14), BPoint(Bounds().right,14) );
	
	StrokeRect( fExpandRect );
	
	if ( fExpanded )
	{
		StrokeLine( BPoint(fExpandRect.left+2, fExpandRect.top+4), BPoint(fExpandRect.right-2,fExpandRect.top+4) );
	} else
	{
		StrokeLine( BPoint(fExpandRect.left+4, fExpandRect.top+2), BPoint(fExpandRect.left+4,fExpandRect.bottom-2) );
		StrokeLine( BPoint(fExpandRect.left+2, fExpandRect.top+4), BPoint(fExpandRect.right-2,fExpandRect.top+4) );
	}

	// title bar
	DrawString( fName.String(), BPoint(3,11) );
}


BArchivable* 
ReplicantView::Instantiate(BMessage* msg)
{
	if( ! validate_instantiation(msg, "ReplicantView") )
		return NULL;
	return new ReplicantView(msg);	
}

void
ReplicantView::AboutRequested()
{
	BAlert* alert = new BAlert("Desktop drawer", "Desktop drawer", "Ok");
	alert->SetShortcut(0, B_ESCAPE);
	alert->Go();
}

void
ReplicantView::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_NODE_MONITOR:
		{ // a file has been added or removed
			msg->PrintToStream();
			
			int32 opcode;
			node_ref node;
			entry_ref ref;

			msg->FindInt32("opcode", &opcode );
			const char * filename = msg->FindString("name");
			
			if ( opcode == B_ENTRY_MOVED )
			{
				printf("Entry moved\n");
				
				ino_t from, to;
				msg->FindInt64("from directory", &from);
				msg->FindInt64("to directory", &to);
				
				if ( from == fNode.node )
				{ // remove
					printf("  remove.\n");
					opcode = B_ENTRY_REMOVED;
				}
				
				if ( to == fNode.node )
				{ // add
					printf("  add.\n");
					opcode = B_ENTRY_CREATED;
					msg->AddInt64("directory",fNode.node);
				}
			}
			
			switch ( opcode )
			{
				case B_ENTRY_CREATED:
				{
					printf("Adding file %s\n", filename);
					
					msg->FindInt32("device",&ref.device);
					msg->FindInt64("directory",&ref.directory);
					ref.set_name(filename);
					
					FileView * v = new FileView(ref);
					
					AddChild(v);
					
					UpdateSize();
				}	break;
				case B_ENTRY_REMOVED:
				{
					printf("Removing file.\n");
					msg->FindInt32("device",&node.device);
					msg->FindInt64("node",&node.node);
					
					for ( int i=0; ChildAt(i); i++ )
					{
						FileView * view = dynamic_cast<FileView*>(ChildAt(i));
						
						if ( !view )
							continue;
						
						if ( view->IsThisNode(node) )
						{
							RemoveChild(view);
							delete view;
						}
					}
					
					UpdateSize();
				}	break;
			}
		}	break;
		
		case B_SIMPLE_DATA:
		{ // something was dropped, move it to fPath if we can
			//msg->PrintToStream();
			entry_ref ref;
			
			BDirectory dir( fPath.String() );
			
			for ( int i=0; msg->FindRef("refs", i, &ref) == B_OK; i++ )
			{
				BEntry entry(&ref);
				
				entry.MoveTo( &dir );
			}
		}	break;

		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		default:
			BView::MessageReceived(msg);
	}
}

void
ReplicantView::AttachedToWindow()
{
	// set up node monitor
	BNode node( fPath.String() );
	if ( node.InitCheck() != B_OK )
	{
		printf("Error settings node to %s\n", fPath.String() );
	}
	node_ref nref;
	node.GetNodeRef(&nref);
	
	status_t res = watch_node( &nref, B_WATCH_DIRECTORY, BMessenger(this) );
	if ( res != B_OK )
	{
		printf("Error watching node.\n");
	}
}

void
ReplicantView::DetachedFromWindow()
{
	stop_watching( BMessenger(this) );
}

void
ReplicantView::MouseDown( BPoint where )
{
	SetMouseEventMask( B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS );
	//Window()->CurrentMessage()->FindInt32("buttons",&m_buttons);
	
	fMouseDown = true;
	fWhere = where;
	
	Invalidate();
}

void
ReplicantView::MouseMoved( BPoint where, uint32 transit, const BMessage * msg )
{
	if ( transit == B_ENTERED_VIEW && msg != NULL )
	{
		fBorderColor = (rgb_color){255,0,0};
		Invalidate();
	}

	if ( transit == B_EXITED_VIEW )
	{
		fBorderColor = (rgb_color){0,0,0};
		Invalidate();
	}

	if ( !fMouseDown )
		return;
	
	BPoint offset( where.x-fWhere.x, where.y-fWhere.y );
	
	MoveBy( offset.x, offset.y );
}

void
ReplicantView::MouseUp( BPoint where )
{
	fMouseDown = false;
	
	if ( fExpandRect.Contains(where) )
	{ // clicked expand-button
		fExpanded = !fExpanded;
		
		if ( fExpanded )
		{
			//ResizeTo( Bounds().Width(), CountChildren()*(ChildAt(0)->Bounds().Height()-1) + 14 );
		} else
		{
			//ResizeTo( Bounds().Width(), 14 );
		}
		
		UpdateSize();
		
		Invalidate();
	}
}

void
ReplicantView::UpdateSize()
{
	list< pair<string,FileView*> > viewlist;
	
	float maxwidth = StringWidth( fName.String() ) + 25;
	
	for ( int i=0; ChildAt(i); i++ )
	{
		FileView * f = dynamic_cast<FileView*>( ChildAt(i) );
		
		if ( !f )
			continue;
		
		viewlist.push_back( pair<string,FileView*>(f->GetName(), f) );
		
		if ( f->Bounds().Width() > maxwidth )
			maxwidth = f->Bounds().Width();
	}
	
	viewlist.sort();
	
	float curry = 16;
	
	for ( list< pair<string,FileView*> >::iterator i = viewlist.begin(); i != viewlist.end(); i++ )
	{
		(*i).second->MoveTo(2,curry);
		curry = (*i).second->Frame().bottom + 2;
	}
	
	if ( fExpanded )
		ResizeTo( maxwidth+4, curry+2 );
	else
		ResizeTo( maxwidth+4, 14 );
	
	
	fExpandRect.OffsetTo( Bounds().right - 11, 3 );
	
	
	Invalidate();
}

// FileView

FileView::FileView(const entry_ref& e)
	: 	BView( BRect(0,0,1,1), "fileView", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	fEntry = e;
	
	Init();
}

FileView::FileView(BMessage* msg)
	: BView(msg)
{
	msg->FindRef("entry", &fEntry);
	
	Init();
}

FileView::~FileView()
{
}

void 
FileView::Init()
{
	char filename[512];
	BPath path;
	
	BEntry entry( &fEntry );
	entry.GetName( filename );
	entry.GetPath( &path );
	
	BNode node(path.Path());
	node.GetNodeRef(&fNode);
	
	LabelView * label = new LabelView( filename );
	
	ResizeTo( label->Bounds().Width() + 3 + 16, 17 );
	
	AddChild( label );
	label->MoveTo(17,1);
	
	BBitmap * bmp = get_icon( BPath(&fEntry).Path() );
	
	ColorView * color = new ColorView(bmp);
	color->SetViewColor( ViewColor() );
	
	AddChild(color);
	color->MoveTo(1,1);
}

void
FileView::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_MOVE_TARGET:
		{ // move target.
			msg->PrintToStream();
			
			BMessage reply(B_MIME_DATA);
			
			entry_ref ref;
			
			msg->FindRef("directory", &ref);
			
			BPath path(&ref);
			path.Append( msg->FindString("name") );
			
			// copy attributes
			BNode node( path.Path() );
			
			char pinfo[1024], xtpinfo[1024];
			
			ssize_t pinfo_size = node.ReadAttr("_trk/pinfo_le", 0x52415754, 0, pinfo, sizeof(pinfo) );
			ssize_t xtpinfo_size = node.ReadAttr("_trk/xtpinfo_le", 0x52415754, 0, xtpinfo, sizeof(xtpinfo) );
			
			// move file
			BEntry entry(&fEntry);
			
			if ( entry.Rename( path.Path(), true ) == B_OK )
			{ // rename failed, revert attributes?
				node.SetTo( path.Path() );
				
				if ( pinfo_size > 0 )
					node.WriteAttr("_trk/pinfo_le", 0x52415754, 0, pinfo, pinfo_size);
	
				if ( xtpinfo_size > 0 )
					node.WriteAttr("_trk/xtpinfo_le", 0x52415754, 0, xtpinfo, xtpinfo_size);
			}
			
			msg->SendReply(&reply);
		}	break;
		
		default:
			BView::MessageReceived(msg);	
	}
}

status_t 
FileView::Archive(BMessage* msg, bool deep) const
{
	return BView::Archive(msg, deep);
}

BArchivable* 
FileView::Instantiate(BMessage* archive)
{
	if( ! validate_instantiation( archive, "FileView" ) )
		return NULL;
	return new FileView(archive);
}

void 
FileView::MouseDown(BPoint where)
{
	SetMouseEventMask( B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS );
	//Window()->CurrentMessage()->FindInt32("buttons",&m_buttons);
	
	fMouseDown = true;
	fIsDrag = false;
	fClickRect = BRect(where,where);
	fClickRect.InsetBy(-3,-3);
	
	Invalidate();
}

void
FileView::MouseMoved( BPoint where, uint32 transit, const BMessage * msg )
{
	if ( fIsDrag )
		return;
	
	if ( !fClickRect.Contains(where) )
	{ // drag.
		fIsDrag = true;
		
		BPath path(&fEntry);
		
		BMessage drag( B_SIMPLE_DATA );
		drag.AddString("be:types", B_FILE_MIME_TYPE );
		drag.AddString("be:filetypes", "application/octet-stream");
		drag.AddString("be:type_descriptions", "A file");
		
		drag.AddInt32("be:actions", B_MOVE_TARGET );
		
		drag.AddString("be:clip_name", path.Leaf() );
		
		DragMessage( &drag, Bounds(), this );
	}
}

void
FileView::MouseUp( BPoint where )
{
	fMouseDown = false;
	
	if ( fIsDrag )
		return;
	
	// click
	be_roster->Launch( &fEntry );
}

bool
FileView::IsThisNode( node_ref & n )
{
	//printf("comparing (%ld - %lld) to (%ld - %lld)\n", fNode.device, fNode.node, n.device, n.node );
	return fNode == n;
}

string
FileView::GetName()
{
	BEntry entry(&fEntry);
	char filename[512];
	entry.GetName(filename);
	
	return filename;
}


// LabelView
void
LabelView::MouseDown(BPoint point)
{
	ConvertToParent(&point);
	((FileView*)Parent())->MouseDown(point);	
}

// ColorView
void 
ColorView::MouseDown(BPoint point)
{
	ConvertToParent(&point);
	((FileView*)Parent())->MouseDown(point);	
}

// eiman
void
ColorView::Draw( BRect rect )
{
	SetDrawingMode( B_OP_COPY );
	SetHighColor( 255,255,255 );
	FillRect( Bounds() );
	
	SetDrawingMode( B_OP_OVER );
	DrawBitmap( fBitmap, BPoint(0,0) );
}

int main()
{
	#ifdef DEBUG
	SET_DEBUG_ENABLED(true);
	#endif
	
	App app;
	app.Run();
	
	return 0;
}
