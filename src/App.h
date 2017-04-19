#ifndef App_h
#define App_h

#include <Entry.h>
#include <Window.h>
#include <Application.h>
#include <StringView.h>
#include <View.h>
#include <Query.h>
#include <Node.h>
#include <String.h>
#include <Bitmap.h>

// eiman
#include <list>
#include <algorithm>
#include <string>

#if BUILD_QUERYWATCHER
	#define IMPEXP __declspec(dllexport)
#else
	#define IMPEXP __declspec(dllimport)
#endif


#define APP_SIG "application/x-vnd.m_eiman.DesktopFolder"

class ReplicantView;
class LabelView;
class ColorView;

class App : public BApplication
{
public:
							App();
							~App();
	
	void 					MessageReceived(BMessage* msg);
	
	
};

class CreatorWindow : public BWindow
{
	public:
		CreatorWindow();
		~CreatorWindow();
		
		void MessageReceived( BMessage * );

		virtual bool QuitRequested();
};

class Window : public BWindow
{
public:
							Window(BRect rect, const char * path);
							~Window();
	
	bool 					QuitRequested();
	void 					MessageReceived(BMessage* msg);
	
	void 					AddQuery(const entry_ref& path, const char* predicate, BRect rect);

private:
	ReplicantView*			fBackground;
};

IMPEXP class ReplicantView : public BView
{
public:
							ReplicantView(BRect frame, const char *);
							ReplicantView(BMessage* msg);
							~ReplicantView();

	status_t 				Archive(BMessage* msg, bool deep=true) const;
	static BArchivable* 	Instantiate(BMessage* msg);
	void 					AboutRequested();
	void 					MessageReceived(BMessage* msg);
	void					Draw( BRect );
	void					MouseDown( BPoint );
	void					MouseMoved( BPoint, uint32, const BMessage * );
	void					MouseUp( BPoint );
	void					Init();
	void					AttachedToWindow();
	void					DetachedFromWindow();
	
	void					UpdateSize();
private:
	BString		fPath;
	BString		fName;
	bool		fExpanded;
	BRect		fExpandRect;
	node_ref	fNode;
	bool		fMouseDown;
	BPoint		fWhere;
	rgb_color	fBorderColor;
};

IMPEXP class FileView : public BView
{
public:
							FileView(const entry_ref& r);
							FileView(BMessage* msg);
							~FileView();
	
	void 					Init();
	void 					MessageReceived(BMessage* msg);
	status_t 				Archive(BMessage* msg, bool deep=true) const;
	static 					BArchivable* Instantiate(BMessage* msg);
	void					MouseDown( BPoint );
	void					MouseMoved( BPoint, uint32, const BMessage * );
	void					MouseUp( BPoint );
	
	bool					IsThisNode( node_ref & );
	
	string					GetName();
	
private:
	entry_ref	fEntry;
	node_ref	fNode;
	
	bool		fMouseDown;
	bool		fIsDrag;
	BRect		fClickRect;
};

IMPEXP class LabelView : public BStringView
{
public:
							LabelView(const char* text)
								: BStringView(BRect(0,0,1,1), "label", text) {
									ResizeTo( StringWidth(text), 15);
								}
/*							LabelView(BMessage* msg)
								: BStringView(msg) {}
	status_t				Archive(BMessage* msg, bool deep = true) const
							{ 	return BStringView::Archive(msg, deep); }
	static BArchivable*		Instantiate(BMessage* msg) 
							{ 	return validate_instantiation(msg, "LabelView") ? new LabelView(msg) : NULL; }							
*/	virtual void			MouseDown(BPoint point);	
};

IMPEXP class ColorView : public BView
{
private:
	BBitmap		* fBitmap;

public:
							ColorView( BBitmap * bmp ) 
								: BView(BRect(0,0,15,15), "color", B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW) {
									fBitmap = bmp;
								}
							
							ColorView(BMessage* msg)
								: BView(msg) {}
							
	status_t				Archive(BMessage* msg, bool deep = true) const
							{ 	return BView::Archive(msg, deep); }
	static BArchivable*		Instantiate(BMessage* msg) { return validate_instantiation(msg, "ColorView") ? new ColorView(msg) : NULL; }
	virtual void			MouseDown(BPoint point);
	
	// eiman
	virtual void			Draw( BRect );
};


#endif
