/* debug routines, and structure dump
 *
 * Copyright ©1996-2008 pinc Software. All Rights Reserved.
 * Licensed under the terms of the GNU General Public License, version 3.
 */


#include "types.h"
#include <stdarg.h>

	 
#ifdef DEBUG


void
panic(const char *format, ...)
{
	char text[256];
	va_list args;
	
	va_start(args, format);
	vsprintf(text, format, args);
	va_end(args);

	bug(text);
	//exit(-1);
}

 
void
check_project_pointer(struct Mappe *map)
{
	struct Mappe *item;

	ASSERT(map != NULL);

	foreach (&gProjects, item) {
		if (map == item)
			// pointer is valid
			return;
	}
	
	panic("map pointer invalid 0x%08lx!\n", map);
}

	 
void
check_page_pointer(struct Page *page)
{
	struct Page *item;

	ASSERT(page != NULL);
	
	check_project_pointer(page->pg_Mappe);

	foreach (&page->pg_Mappe->mp_Pages, item) {
		if (page == item)
			// pointer is valid
			return;
	}
	
	panic("page pointer is invalid 0x%08lx!\n", page);
}


void
dump_diagrams(struct Page *page)
{
	struct gDiagram *diagram;

	check_page_pointer(page);

	bug("diagrams for page \"%s\" (0x%08lx)\n", page->pg_Node.ln_Name, page);

	foreach (&page->pg_gDiagrams, diagram) {
		bug("\t0x%08lx\n", diagram);
	}
}


void
dump_object(struct gObject *object, bool nullPageAllowed)
{
	struct Link *link;

	bug("gObject at 0x%08lx\n", object);
	bug("\tname = \"%s\"\n", object->go_Node.ln_Name);
	bug("\tpos = %ld\n", object->go_Pos);
	bug("\ttype = %d\n", object->go_Type);
	bug("\tflags = %d\n", object->go_Flags);
	bug("\tpage = 0x%08lx\n", object->go_Page);
	bug("\twindow = 0x%08lx\n", object->go_Window);
	bug("\tundo = 0x%08lx\n", object->go_Undo);
	bug("\tclass = 0x%08lx\n", object->go_Class);
	
	bug("\treferenced by:\n");
	
	foreach (&object->go_ReferencedBy, link)
		bug("\t  0x%08lx -> 0x%08lx\n", link, link->l_Link);

	if (!nullPageAllowed || object->go_Page != NULL)
		check_page_pointer(object->go_Page);
}


void
dump_diagram(struct gDiagram *diagram, bool nullPageAllowed)
{
	struct gLink *link;

	bug("gDiagram at 0x%08lx\n", diagram);

	dump_object(diagram, nullPageAllowed);
	bug("\t--- diagram ---\n");

	if ((diagram->gd_Object.go_Class->gc_Node.in_Type & GCT_DIAGRAM) == 0) {
		bug("\tdump_diagram() called on normal object!\n");
		return;
	}

	bug("\tflags = %d\n", diagram->gd_Flags);
	bug("\tread data = %d\n", diagram->gd_ReadData);
	bug("\trange = \"%s\" (0x%08lx)\n", diagram->gd_Range, diagram->gd_Range);
	bug("\treference = 0x%08lx\n", diagram->gd_Reference);

	foreach (&diagram->gd_Values, link) {
		bug("\t  0x%08lx value = %ld\n", link, (long)link->gl_Value);
	}
	bug("\tlinks = 0x%08lx\n", diagram->gd_Links);
	bug("\tlegendX = 0x%08lx\n", diagram->gd_LegendX);
	bug("\tlegendY = 0x%08lx\n", diagram->gd_LegendY);
	bug("\tcols = %ld\n", diagram->gd_Cols);
	bug("\trows = %ld\n", diagram->gd_Rows);
	bug("\tdata page = 0x%08lx\n", diagram->gd_DataPage);
	bug("\tpage number = %ld\n", diagram->gd_PageNumber);
}

 
#endif	// only if DEBUG
 
