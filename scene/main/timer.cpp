/*************************************************************************/
/*  timer.cpp                                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "timer.h"

void Timer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (autostart) {
#ifdef TOOLS_ENABLED
				if (Engine::get_singleton()->is_editor_hint() && get_tree()->get_edited_scene_root() && (get_tree()->get_edited_scene_root() == this || get_tree()->get_edited_scene_root()->is_ancestor_of(this))) {
					break;
				}
#endif
				start();
				autostart = false;
			}
		} break;

		case NOTIFICATION_INTERNAL_PROCESS: {
			if (!processing || timer_process_callback == TIMER_PROCESS_PHYSICS || !is_processing_internal()) {
				return;
			}
			time_left -= get_process_delta_time();

			if (time_left < 0) {
				if (max_repeats == -1 || repeat_index < (max_repeats - 1)) {
					time_left += wait_time;
					repeat_index += 1;
				} else {
					stop();
				}
				emit_signal(SNAME("timeout"));
			}
		} break;

		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (!processing || timer_process_callback == TIMER_PROCESS_IDLE || !is_physics_processing_internal()) {
				return;
			}
			time_left -= get_physics_process_delta_time();

			if (time_left < 0) {
				if (max_repeats == -1 || repeat_index < (max_repeats - 1)) {
					time_left += wait_time;
					repeat_index += 1;
				} else {
					stop();
				}
				emit_signal(SNAME("timeout"));
			}
		} break;
	}
}

void Timer::set_wait_time(double p_time) {
	ERR_FAIL_COND_MSG(p_time <= 0, "Time should be greater than zero.");
	wait_time = p_time;
	update_configuration_warnings();
}

double Timer::get_wait_time() const {
	return wait_time;
}

void Timer::set_max_repeats(int p_max_repeats) {
	ERR_FAIL_COND_MSG(p_max_repeats < -1, "Max repeats should be equal to or greater than -1.");
	max_repeats = p_max_repeats;
	update_configuration_warnings();
}

int Timer::get_max_repeats() const {
	return max_repeats;
}

void Timer::set_autostart(bool p_start) {
	autostart = p_start;
}

bool Timer::has_autostart() const {
	return autostart;
}

void Timer::start(double p_time) {
	ERR_FAIL_COND_MSG(!is_inside_tree(), "Timer was not added to the SceneTree. Either add it or set autostart to true.");

	if (p_time > 0) {
		set_wait_time(p_time);
	}
	repeat_index = -1;
	time_left = wait_time;
	_set_process(true);
}

void Timer::stop() {
	time_left = -1;
	repeat_index = max_repeats;
	_set_process(false);
	autostart = false;
}

void Timer::set_paused(bool p_paused) {
	if (paused == p_paused) {
		return;
	}

	paused = p_paused;
	_set_process(processing);
}

bool Timer::is_paused() const {
	return paused;
}

bool Timer::is_stopped() const {
	return get_time_left() <= 0;
}

double Timer::get_time_left() const {
	return time_left > 0 ? time_left : 0;
}

int Timer::get_repeat_index() const {
	return repeat_index;
}

int Timer::get_repeats_left() const {
	return max_repeats > 0 ? (max_repeats - repeat_index) : max_repeats;
}

void Timer::set_timer_process_callback(TimerProcessCallback p_callback) {
	if (timer_process_callback == p_callback) {
		return;
	}

	switch (timer_process_callback) {
		case TIMER_PROCESS_PHYSICS:
			if (is_physics_processing_internal()) {
				set_physics_process_internal(false);
				set_process_internal(true);
			}
			break;
		case TIMER_PROCESS_IDLE:
			if (is_processing_internal()) {
				set_process_internal(false);
				set_physics_process_internal(true);
			}
			break;
	}
	timer_process_callback = p_callback;
}

Timer::TimerProcessCallback Timer::get_timer_process_callback() const {
	return timer_process_callback;
}

void Timer::_set_process(bool p_process, bool p_force) {
	switch (timer_process_callback) {
		case TIMER_PROCESS_PHYSICS:
			set_physics_process_internal(p_process && !paused);
			break;
		case TIMER_PROCESS_IDLE:
			set_process_internal(p_process && !paused);
			break;
	}
	processing = p_process;
}

PackedStringArray Timer::get_configuration_warnings() const {
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (wait_time < 0.05 - CMP_EPSILON) {
		warnings.push_back(RTR("Very low timer wait times (< 0.05 seconds) may behave in significantly different ways depending on the rendered or physics frame rate.\nConsider using a script's process loop instead of relying on a Timer for very low wait times."));
	}

	return warnings;
}

void Timer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_wait_time", "time_sec"), &Timer::set_wait_time);
	ClassDB::bind_method(D_METHOD("get_wait_time"), &Timer::get_wait_time);

	ClassDB::bind_method(D_METHOD("set_max_repeats", "enable"), &Timer::set_max_repeats);
	ClassDB::bind_method(D_METHOD("get_max_repeats"), &Timer::get_max_repeats);

	ClassDB::bind_method(D_METHOD("set_autostart", "enable"), &Timer::set_autostart);
	ClassDB::bind_method(D_METHOD("has_autostart"), &Timer::has_autostart);

	ClassDB::bind_method(D_METHOD("start", "time_sec"), &Timer::start, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("stop"), &Timer::stop);

	ClassDB::bind_method(D_METHOD("set_paused", "paused"), &Timer::set_paused);
	ClassDB::bind_method(D_METHOD("is_paused"), &Timer::is_paused);

	ClassDB::bind_method(D_METHOD("is_stopped"), &Timer::is_stopped);

	ClassDB::bind_method(D_METHOD("get_time_left"), &Timer::get_time_left);

	ClassDB::bind_method(D_METHOD("get_repeat_index"), &Timer::get_repeat_index);
	ClassDB::bind_method(D_METHOD("get_repeats_left"), &Timer::get_repeats_left);

	ClassDB::bind_method(D_METHOD("set_timer_process_callback", "callback"), &Timer::set_timer_process_callback);
	ClassDB::bind_method(D_METHOD("get_timer_process_callback"), &Timer::get_timer_process_callback);

	ADD_SIGNAL(MethodInfo("timeout"));

	ADD_PROPERTY(PropertyInfo(Variant::INT, "process_callback", PROPERTY_HINT_ENUM, "Physics,Idle"), "set_timer_process_callback", "get_timer_process_callback");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wait_time", PROPERTY_HINT_RANGE, "0.001,4096,0.001,or_greater,exp,suffix:s"), "set_wait_time", "get_wait_time");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_repeats", PROPERTY_HINT_RANGE, "-1,1000,1,or_greater"), "set_max_repeats", "get_max_repeats");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autostart"), "set_autostart", "has_autostart");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "paused", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_paused", "is_paused");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time_left", PROPERTY_HINT_NONE, "suffix:s", PROPERTY_USAGE_NONE), "", "get_time_left");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "repeat_index", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "", "get_repeat_index");

	BIND_ENUM_CONSTANT(TIMER_PROCESS_PHYSICS);
	BIND_ENUM_CONSTANT(TIMER_PROCESS_IDLE);
}

Timer::Timer() {}
