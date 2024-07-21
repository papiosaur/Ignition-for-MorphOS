/* OeffneInterface.c */


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gtdrag.h>

struct Library   *GTDragBase;
struct DemoIFace *IGTDrag;

int main()
{
  if((GTDragBase = (struct Library *) OpenLibrary("gtdrag.library",50)))
  {
    if((IGTDrag = (struct DemoIFace *) GetInterface((struct Library *)GTDragBase,"main",1,NULL)))
    {
      Printf("Hallo, alles klar\n");

      DropInterface((struct Interface *)IGTDrag);
    }
    else Printf("kein main Interface in der gtdrag.library\n");

    CloseLibrary((struct Library *)GTDragBase);
  }
  else Printf("es fehlt gtdrag.library\n");

  return( 0 );
}

