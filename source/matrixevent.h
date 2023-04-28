#ifndef _MATRIXEVENT_H_
#define _MATRIXEVENT_H_

#include <switch.h>
#include <string>
#include <jansson.h>
#include "room.h"
#include <matrixclient.h>

class Room;

enum struct EventType : u8 {
	invalid,
	m_room_message,
	m_room_member,
	m_room_name,
	m_room_topic,
	m_room_avatar,
	m_room_redaction,
};

enum struct EventMsgType : u8 {
	m_text,
	m_notice,
	m_emote,
	m_file,
	m_image,
	m_video,
	m_audio,
};

struct EventRoomMessage {
	EventMsgType msgtype;
	std::string body;
	std::string editEventId;
};

enum struct EventMemberMembership : u8 {
	invite,
	join,
	leave,
	ban,
};

struct EventRoomMember {
	Matrix::MemberInfo info;
	std::string stateKey;
	EventMemberMembership membership;
};

struct EventRoomName {
	std::string name;
};

struct EventRoomTopic {
	std::string topic;
};

struct EventRoomAvatar {
	std::string avatarUrl;
};

struct EventRoomRedaction {
	std::string redacts;
};

class MatrixEvent {
private:
	Room* room = NULL;
	
	std::string getDisplayName(std::string id);
public:
	EventType type;
	std::string sender;
	std::string eventId;
	u64 originServerTs;
	
	bool read = false;
	
	union {
		EventRoomMessage* message;
		EventRoomMember* member;
		EventRoomName* roomName;
		EventRoomTopic* roomTopic;
		EventRoomAvatar* roomAvatar;
		EventRoomRedaction* redaction;
	};
	
	MatrixEvent(FILE* fp);
	MatrixEvent(json_t* event);
	~MatrixEvent();
	void setRoom(Room* r);
	void print();
	bool isValid();
	void writeToFile(FILE* fp);
	void readFromFile(FILE* fp);
};

#endif // _MATRIXEVENT_H_
