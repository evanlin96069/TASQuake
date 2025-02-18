#pragma once
#include "libtasquake/io.hpp"
#include "libtasquake/insertion_order_map.hpp"
#include <unordered_map>
#include <string>
#include <vector>

enum class HookEnum
{
	Frame,          // Execute like a normal command right after the previous one
	LevelChange,	// Execute when level change has completed
	ScriptCompleted // Script run completed
};

struct TestBlock
{
	HookEnum hook; // The hook used for the test block
	int hook_count; // How many iterations of this hook should complete before performing the action
	unsigned int afterframes_filter; // Filter for the afterframes command
	std::string command; // The command to be executed


	void Write_To_Stream(std::ostream& os);
	TestBlock(const std::string& line);
	TestBlock();
};

struct FrameBlock
{
	FrameBlock();
	bool parsed = false;
	int frame = 0;
	TASQuake::ordered_map<float> convars;
	TASQuake::ordered_map<bool> toggles;
	std::vector<std::string> commands;

	void Stack(const FrameBlock& new_block);
	std::string GetCommand() const;
	void Add_Command(const std::string& line);
	void Parse_Frame_No(const std::string& line, int& running_frame);
	void Parse_Convar(const std::string& line, size_t& spaceIndex, size_t& startIndex);
	void Parse_Toggle(const std::string& line);
	void Parse_Command(const std::string& line);
	void Parse_Line(const std::string& line, int& running_frame);
	void Reset();

	bool HasToggleValue(const std::string& cmd, bool value) const;
	bool HasCvarValue(const std::string& cmd, float value) const;
	bool HasToggle(const std::string& cmd) const;
	bool HasConvar(const std::string& cvar) const;
};

class TASScript
{
public:
	TASScript();
	TASScript(const char* file_name);
	void ApplyChanges(const TASScript* script, int& first_changed_frame);
	bool Load_From_Memory(TASQuakeIO::BufferReadInterface& iface);
	void Write_To_Memory(TASQuakeIO::BufferWriteInterface& iface) const;
	bool Load_From_File();
	void Write_To_File() const;
	bool Load_From_String(const char* input);
	std::vector<FrameBlock> blocks;
	std::string file_name;
	std::string ToString() const;
	mutable int prev_block_number = 0;
	void Prune(int min_frame, int max_frame);
	void Prune(int min_frame);
	void RemoveBlocksAfterFrame(int frame);
	void RemoveCvarsFromRange(const std::string& name, int min_frame, int max_frame);
	void RemoveTogglesFromRange(const std::string& name, int min_frame, int max_frame);
	void AddScript(const TASScript* script, int frame);
	bool ShiftSingleBlock(size_t blockIndex, int delta);
	bool ShiftBlocks(size_t blockIndex, int delta);
	int GetBlockIndex(int frame) const;
	void AddCvar(const std::string& cmd, float value, int frame);
	void AddToggle(const std::string& cmd, bool state, int frame);
	void AddCommand(const std::string& cmd, int frame);
	bool AddShot(float pitch, float yaw, int frame, int turn_frames); // Returns true if script changed
	void RemoveShot(int frame, int turn_frames);
	const FrameBlock* Get_Frameblock(int frame) const;
private:
	bool _Load_From_File(TASQuakeIO::ReadInterface& readInterface);
	void _Write_To_File(TASQuakeIO::WriteInterface& writeInterface) const;
};

class TestScript
{
public:
	TestScript();						// Default constructor is only intended for containers
	TestScript(const char* file_name);	// Initialize script with c-string filename
	bool Load_From_File();				// Load the script from file
	void Write_To_File();				// Write the script to file
	std::vector<TestBlock> blocks;		// Testblocks in the test
	TestBlock exit_block;				// Testblock to be run after the test is finished
	std::string file_name;				// Filename of the test
	std::string description;			// Description on what the test is testing
};
