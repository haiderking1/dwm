#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
int main(void) {
 Display *d=XOpenDisplay(NULL); Window root,parent,*children; unsigned int n,i; char *name;
 if(!d) return 1;
 root=DefaultRootWindow(d);
 if(!XQueryTree(d,root,&root,&parent,&children,&n)) return 1;
 for(i=0;i<n;i++) {
  XWindowAttributes a; Atom type; int format; unsigned long count,left; unsigned char *data=NULL;
  if(!XGetWindowAttributes(d,children[i],&a)) continue;
  name=NULL; XFetchName(d,children[i],&name);
  printf("0x%lx %dx%d%+d%+d map=%d override=%d name=%s",children[i],a.width,a.height,a.x,a.y,a.map_state,a.override_redirect,name?name:"");
  XGetWindowProperty(d,children[i],XInternAtom(d,"_NET_WM_WINDOW_TYPE",False),0,16,False,XA_ATOM,&type,&format,&count,&left,&data);
  if(data && format==32) { unsigned long j; for(j=0;j<count;j++){char *s=XGetAtomName(d,((Atom*)data)[j]); printf(" type=%s",s); XFree(s);} }
  XFree(data); data=NULL;
  XGetWindowProperty(d,children[i],XInternAtom(d,"_NET_WM_STRUT_PARTIAL",False),0,12,False,XA_CARDINAL,&type,&format,&count,&left,&data);
  if(data && format==32) { unsigned long j; printf(" strut=");for(j=0;j<count;j++) printf("%lu,",((unsigned long*)data)[j]); }
  puts(""); XFree(data); XFree(name);
 }
 XFree(children);XCloseDisplay(d);return 0;
}
