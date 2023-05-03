#include <switch.h>
#include <stdio.h>
#include <matrixclient.h>

#include <string>
#include <string.h>
#include <map>
#include <vector>
#include <bits/stdc++.h> 
#include "store.h"
#include "room.h"
#include "defines.h"
#include "request.h"
#include "roomcollection.h"
#include "util.h"

PrintConsole* topScreenConsole;
PrintConsole* bottomScreenConsole;

bool renderRoomDisplay = true;
Room* currentRoom;

enum struct State {
	roomPicking,
	roomDisplaying,
};
State state = State::roomPicking;

Matrix::Client* client;

void sync_new_event(std::string roomId, json_t* event) {
	roomCollection->ensureExists(roomId);
	MatrixEvent* evt = new MatrixEvent(event);
	if (!evt->isValid()) {
		delete evt;
		return;
	}
	Room* room = roomCollection->get(roomId);
	if (room) {
		room->addEvent(evt);
	}
}

void sync_leave_room(std::string roomId, json_t* event) {
	roomCollection->remove(roomId);
}

void sync_room_info(std::string roomId, Matrix::RoomInfo roomInfo) {
	roomCollection->setInfo(roomId, roomInfo);
}

void sync_room_limited(std::string roomId, std::string nextBatch) {
	Room* room = roomCollection->get(roomId);
	if (room) {
		room->clearEvents();
	}
}

std::string getMessage() {
	SwkbdConfig swkbd;
	char mybuf[1024];
	swkbdCreate(&swkbd, 0);
	swkbdConfigMakePresetDefault(&swkbd);
	swkbdConfigSetGuideText(&swkbd, "Enter message...");
	swkbdShow(&swkbd, mybuf, sizeof(mybuf));
	swkbdClose(&swkbd);
	return mybuf;
}

std::string getHomeserverUrl() {
	SwkbdConfig swkbd;
	char mybuf[120];
	swkbdCreate(&swkbd, 0);
	swkbdConfigMakePresetDefault(&swkbd);
	swkbdConfigSetGuideText(&swkbd, "Homeserver URL");
	swkbdShow(&swkbd, mybuf, sizeof(mybuf));
	swkbdClose(&swkbd);
	return mybuf;
}

std::string getUsername() {
	SwkbdConfig swkbd;
	char mybuf[120];
	swkbdCreate(&swkbd, 0);
	swkbdConfigMakePresetDefault(&swkbd);
	swkbdConfigSetGuideText(&swkbd, "Username");
	swkbdShow(&swkbd, mybuf, sizeof(mybuf));
	swkbdClose(&swkbd);
	return mybuf;
}

std::string getPassword() {
	SwkbdConfig swkbd;
	char mybuf[120];
	swkbdCreate(&swkbd, 0);
	swkbdConfigMakePresetPassword(&swkbd);
	swkbdConfigSetGuideText(&swkbd, "Password");
	swkbdShow(&swkbd, mybuf, sizeof(mybuf));
	swkbdClose(&swkbd);
	return mybuf;
}

void loadRoom() {
	printf("Loading room %s...\n", currentRoom->getDisplayName().c_str());
	renderRoomDisplay = true;
	state = State::roomDisplaying;
	printf("==================================================\n");
	currentRoom->printEvents();
}

int roomPickerTop = 0;
int roomPickerItem = 0;
void roomPicker(PadState &pad) {
	u64 kDown = padGetButtonsDown(&pad); // CHANGE
	bool renderRooms = false;
	if (kDown & HidNpadButton_AnyDown || kDown & HidNpadButton_AnyRight) {
		if (kDown & HidNpadButton_AnyDown) {
			roomPickerItem++;
		} else {
			roomPickerItem += 25;
		}
		if (roomPickerItem >= roomCollection->size()) {
			roomPickerItem = roomCollection->size() - 1;
		}
		while (roomPickerItem - roomPickerTop > 44) {
			roomPickerTop++;
		}
		renderRooms = true;
	}
	if (kDown & HidNpadButton_AnyUp || kDown & HidNpadButton_AnyLeft) {
		if (kDown & HidNpadButton_AnyUp) {
			roomPickerItem--;
		} else {
			roomPickerItem -= 25;
		}
		if (roomPickerItem < 0) {
			roomPickerItem = 0;
		}
		while (roomPickerItem - roomPickerTop < 0) {
			roomPickerTop--;
		}
		renderRooms = true;
	}
	if (kDown & HidNpadButton_A) {
		currentRoom = roomCollection->getByIndex(roomPickerItem);
		if (currentRoom) {
			loadRoom();
		}
		return;
	}
	roomCollection->maybePrintPicker(roomPickerTop, roomPickerItem, renderRooms);
}

void displayRoom(PadState &pad) {
	if (currentRoom->haveDirty()) {
		currentRoom->printEvents();
	}
	u64 kDown = padGetButtonsDown(&pad); // CHANGE
	if (kDown & HidNpadButton_B) {
		state = State::roomPicking;
		printf("==================================================\n");
		roomCollection->maybePrintPicker(roomPickerTop, roomPickerItem, true);
		return;
	}
	if (kDown & HidNpadButton_A) {
		request->setTyping(currentRoom->getId(), true);
		std::string message = getMessage();
		request->setTyping(currentRoom->getId(), false);
		if (message != "") {
			request->sendText(currentRoom->getId(), message);
		}
	}
	if (!renderRoomDisplay && !currentRoom->haveDirtyInfo()) {
		return;
	}
	//printf("\x1b[2J");
	renderRoomDisplay = false;
	currentRoom->printInfo();
	printf("\nPress A to send a message\nPress B to go back\n");
}

bool setupAcc(PadState &pad) {
	std::string homeserverUrl = store->getVar("hsUrl");
	std::string token = store->getVar("token");
	std::string userId = "";
	if (homeserverUrl != "" || token != "") {
		client = new Matrix::Client(homeserverUrl, token, store);
		userId = client->getUserId();
		if (userId != "") {
			printf("Logged in as %s\n", userId.c_str());
			return true;
		}
		delete client;
		client = NULL;
		printf("Invalid token\n");
	}
	printf("Press A to log in\n");
	while(appletMainLoop()) {
		padUpdate(&pad);
		u64 kDown = padGetButtonsDown(&pad); // CHANGE
		if (kDown & HidNpadButton_A) break;
		if (kDown & HidNpadButton_Plus) return false;
		consoleUpdate(NULL);
	}

	homeserverUrl = getHomeserverUrl();

	client = new Matrix::Client(homeserverUrl, "", store);

	std::string username = getUsername();
	std::string password = getPassword();

	if (!client->login(username, password)) {
		return false;
	}

	userId = client->getUserId();
	printf("Logged in as %s\n", userId.c_str());
	store->setVar("hsUrl", homeserverUrl);
	store->setVar("token", client->getToken());
	return true;
}

void clearCache() {
	printf("Clearing cache...\n");
	store->delVar("synctoken");
	store->delVar("roomlist");
	remove_directory("rooms");
}

void logout() {
	printf("Logging out...\n");
	client->logout();
	store->delVar("token");
	store->delVar("hsUrl");
	clearCache();
}

int main(int argc, char** argv) {
	consoleInit(NULL);

	store->init();

	printf("Miitrix-NX\n");

	padConfigureInput(1, HidNpadStyleSet_NpadStandard);

	PadState pad;
    padInitializeDefault(&pad);
	
	while (!setupAcc(pad)) {
		printf("Failed to log in!\n\n");
		printf("Press START to exit.\n");
		printf("Press SELECT to try again.\n\n");
	
		while (appletMainLoop()) {
			padUpdate(&pad);
			u64 kDown = padGetButtonsDown(&pad);

			if (kDown & HidNpadButton_Plus) {
				consoleExit(NULL);
				return 0;
			}
			if (kDown & HidNpadButton_Minus) {
				break;
			}

			consoleUpdate(NULL);
		}
	}

	client->setEventCallback(&sync_new_event);
	client->setLeaveRoomCallback(&sync_leave_room);
	client->setRoomInfoCallback(&sync_room_info);
	client->setRoomLimitedCallback(&sync_room_limited);
	
	printf("Loading channel list...\n");

	roomCollection->readFromFiles();
	request->start();
	client->startSyncLoop();

	while (appletMainLoop()) {
		//printf("%d\n", i++);
		//Scan all the inputs. This should be done once for each frame
		padUpdate(&pad);

		roomCollection->frameAllDirty();

		if (state == State::roomPicking) {
			roomPicker(pad);
		} else if (state == State::roomDisplaying) {
			displayRoom(pad);
		}

		roomCollection->writeToFiles();
		roomCollection->resetAllDirty();
		
		u64 kDown = padGetButtonsDown(&pad);
		if (kDown & HidNpadButton_Plus) {
			u64 kHeld = padGetButtons(&pad);
			if (kHeld & HidNpadButton_X) {
				clearCache();
			}
			if (kHeld & HidNpadButton_B) {
				logout();
			}
			break; // break in order to return to hbmenu
		}
		consoleUpdate(NULL);
	}
//	client->stopSyncLoop();

	consoleExit(NULL);
	return 0;
}
