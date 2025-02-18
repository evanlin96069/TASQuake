#include "libtasquake/script_playback.hpp"
#include "libtasquake/utils.hpp"

PlaybackInfo::PlaybackInfo()
{
	pause_frame = 0;
	current_frame = 0;
	script_running = false;
	should_unpause = false;
}

const FrameBlock* PlaybackInfo::Get_Current_Block(int frame) const
{
	int blck = GetBlockNumber(frame);

	if (blck >= current_script.blocks.size())
		return nullptr;
	else
		return &current_script.blocks[blck];
}

const FrameBlock* PlaybackInfo::Get_Stacked_Block() const
{
	return &stacked;
}

int PlaybackInfo::GetBlockNumber(int frame) const
{
	if (frame == -1)
		frame = current_frame;

	return current_script.GetBlockIndex(frame);
}

int PlaybackInfo::Get_Number_Of_Blocks() const
{
	return current_script.blocks.size();
}

int PlaybackInfo::Get_Last_Frame() const
{
	if (current_script.blocks.empty())
		return 0;
	else
	{
		return current_script.blocks[current_script.blocks.size() - 1].frame;
	}
}

bool PlaybackInfo::In_Edit_Mode() const
{
	return !script_running && TASQuake::GamePaused() && !current_script.blocks.empty();
}

void PlaybackInfo::CalculateStack()
{
	stacked.Reset();

	for (auto& block : current_script.blocks)
	{
		if (block.frame >= current_frame)
			break;

		stacked.Stack(block);
	}
}

PlaybackInfo PlaybackInfo::GetTimeShiftedVersion(const PlaybackInfo* info, int start_frame) {
	PlaybackInfo output;
	output.current_script.file_name = info->current_script.file_name;
	int old_start_frame = info->current_frame;
	if(start_frame == -1) {
		start_frame = info->current_frame;
	}

	FrameBlock stacked;
	stacked.frame = 0;
	stacked.parsed = true;
	bool added_stack = false;

	for (int i = 0; i < info->current_script.blocks.size(); ++i)
	{
		FrameBlock block = info->current_script.blocks[i];
		if (block.frame <= start_frame) {
			stacked.Stack(block);
			if(block.frame == start_frame) {
				stacked.commands = block.commands;
			}
		} else {
			if(!added_stack) {
				output.current_script.blocks.push_back(stacked);
				added_stack = true;
			}

			block.frame -= start_frame;
			output.current_script.blocks.push_back(block);
		}
	}

	if(!added_stack) {
		output.current_script.blocks.push_back(stacked);
		added_stack = true;
	}
	
	return output;
}
