inherit "/std/monster";

#include "../domain.h"

void setup(void) {
   set_name("cow");

   /* Leave out the gender specification, get a random one */
   /*  set_gender( "male" ); */

   set_short("Dirty COW");
   set_long("This is an ancient vulnerability, no idea how its still here.");
   set_race("vuln");
   set_level(3);
   set_hit_skill("combat/unarmed");
   set_skill("combat/unarmed", 20);
   set_skill("combat/defense", 1);
   add_object(DIR + "/objects/con_potion.c", 9);
}
