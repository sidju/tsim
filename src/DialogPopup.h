
#ifndef TS_DialogPopup_H_
#define TS_DialogPopup_H_

void DialogPopup(String name, Widget parent, 
			Boolean (*acknowledge)(), XtPointer proc_data,
			String *button_list, Cardinal no_of_buttons, 
			String default_value);

#endif /* TS_DialogPopup_H_ */

