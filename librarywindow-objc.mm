#include "ui_librarywindow.h"
#include "librarywindow.h"

#include <QDebug>
#include <QBoxLayout>
#include <QMacCocoaViewContainer>
#import <AppKit/AppKit.h>

@interface MainWindowDelegate: NSResponder <NSWindowDelegate>
    @property (strong) NSObject *qtDelegate;
    @property Ui::LibraryWindow *libraryWindowUi;
    @property NSWindow *window;
@end


@implementation MainWindowDelegate

- (id)init:(Ui::LibraryWindow*)ui {
   if (self = [super init]) {
       //Set main window UI to call touch bar handlers
       self.libraryWindowUi = ui;
   }

   return self;
}

- (void)installAsDelegateForWindow:(NSWindow *)window
{
   _qtDelegate = window.delegate; // Save current delegate for forwarding
   window.delegate = self;
   self.window = window;
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
   // We want to forward to the qt delegate. Respond to selectors it
   // responds to in addition to selectors this instance resonds to.
   return [_qtDelegate respondsToSelector:aSelector] || [super respondsToSelector:aSelector];
}

- (void)forwardInvocation:(NSInvocation *)anInvocation
{
   // Forward to the existing delegate. This function is only called for selectors
   // this instance does not responds to, which means that the qt delegate
   // must respond to it (due to the respondsToSelector implementation above).
   [anInvocation invokeWithTarget:_qtDelegate];
}

- (NSApplicationPresentationOptions)window:(NSWindow *)window willUseFullScreenPresentationOptions:(NSApplicationPresentationOptions)proposedOptions {
   //On an unrelated note, set full screen window properties
   return (NSApplicationPresentationFullScreen | NSApplicationPresentationHideDock | NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationAutoHideToolbar);
}

- (void)windowDidEnterFullScreen:(NSNotification*)notification {
    NSWindowStyleMask styleMask = NSWindowStyleMaskFullSizeContentView|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskFullScreen;
    [_window setStyleMask:styleMask];
}

- (void)windowDidExitFullScreen:(NSNotification*)notification {
    NSWindowStyleMask styleMask = NSWindowStyleMaskFullSizeContentView|NSWindowStyleMaskResizable|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable;
    [_window setStyleMask:styleMask];
}

@end

void LibraryWindow::setupMacOS() {
    NSView *view = reinterpret_cast<NSView *>(this->winId());
    MainWindowDelegate *winDelegate = [[MainWindowDelegate alloc] init:ui];
    [winDelegate installAsDelegateForWindow:view.window];

    NSWindowStyleMask styleMask = NSWindowStyleMaskFullSizeContentView|NSWindowStyleMaskResizable|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable;

    [view.window setTitlebarAppearsTransparent:YES];
    [view.window setTitleVisibility:NSWindowTitleHidden];
    [view.window setStyleMask:styleMask];
//    [view.window setMovableByWindowBackground:YES];

    QBoxLayout* windowControlsLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    windowControlsLayout->setContentsMargins(6, 0, 0, 0);
    windowControlsLayout->setSpacing(6);

    NSButton* closeButton = [NSWindow standardWindowButton:NSWindowCloseButton forStyleMask:styleMask];
    QMacCocoaViewContainer* closeWidget = new QMacCocoaViewContainer(closeButton, this);
    closeWidget->setFixedSize(14, 16);
    windowControlsLayout->addWidget(closeWidget);

    NSButton* minButton = [NSWindow standardWindowButton:NSWindowMiniaturizeButton forStyleMask:styleMask];
    QMacCocoaViewContainer* minWidget = new QMacCocoaViewContainer(minButton, this);
    minWidget->setFixedSize(14, 16);
    windowControlsLayout->addWidget(minWidget);

    NSButton* fsButton = [NSWindow standardWindowButton:NSWindowZoomButton forStyleMask:styleMask];
    QMacCocoaViewContainer* fsWidget = new QMacCocoaViewContainer(fsButton, this);
    fsWidget->setFixedSize(14, 16);
    windowControlsLayout->addWidget(fsWidget);

    static_cast<QBoxLayout*>(ui->headerWidget->layout())->insertLayout(0, windowControlsLayout);
}

void LibraryWindow::macOSDrag(QMouseEvent* event) {
    CGEventRef cgEvent = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown, ui->headerBar->mapToGlobal(event->pos()).toCGPoint(), kCGMouseButtonLeft);
    NSEvent* nsEvent = [NSEvent eventWithCGEvent:cgEvent];

    NSView *view = reinterpret_cast<NSView *>(this->winId());
    [view.window performWindowDragWithEvent:nsEvent];
    CFRelease(cgEvent);
}
