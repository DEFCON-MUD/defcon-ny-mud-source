#include "../domain.h"

inherit "/std/object";

void setup(void) {
   set_id("potion");
   set_adj("small", "skill", "purple");
   set_short("A small purple potion");
   set_long("A small fancy bottle, with a faintly glowing purple liquid in it.  " +
      "A label on the bottle says ode de skills.\n");
   set_eatable(1);
   set_gettable(1);
   set_value(10);
   set_weight(2);
}

int do_eat(void) {
   int x;

   x = random(5) + 1;
   write("You drink the potion.  You feel a warmth in your chest.\n");
   this_player()->query_environment()->tell_room(this_player(),
      this_player()->query_Name() + " drinks a potion of skills.\n");
   this_player()->increase_hp(x);
   this_player()->increase_expr(5000);
   this_player()->set_skill("combat/sharp/large", 100);
   this_player()->set_skill("combat/defense", 100);
   this_player()->set_skill("combat/unarmed", 100);
   this_player()->set_skill("combat/blunt/small", 100);
   this_object()->destruct();
}
