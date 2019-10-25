#ifndef _ROOMCOLLECTION_H_
#define _ROOMCOLLECTION_H_

#include <string>
#include <vector>
#include "room.h"
#include <matrixclient.h>

class RoomCollection {
private:
	std::vector<Room*> rooms;
	void order();
	bool dirtyOrder = true;
public:
	int size();
	Room* get(std::string roomId);
	Room* getByIndex(int i);
	void ensureExists(std::string roomId);
	void setInfo(std::string roomId, Matrix::RoomInfo info);
	void remove(std::string roomId);
	void maybePrintPicker(int pickerTop, int pickerItem, bool override);
	void resetAllDirty();
	void writeToFiles();
};

extern RoomCollection* roomCollection;

#endif // _ROOMCOLLECTION_H_