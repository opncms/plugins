////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2013-2018 Vladimir Yakunin (kpeo) <opncms@gmail.com>
//
//  The redistribution terms are provided in the COPYRIGHT.txt file
//  that must be distributed with this source code.
//
////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef DATA_user_H
#define DATA_user_H

#include <cppcms/view.h>
#include <cppcms/form.h>
#include <booster/function.h>
#include <opncms/base.h>

#define USER_MIN_NANE 6
#define USER_MIN_EMAIL 8
#define USER_MIN_PASSWORD 8

namespace content {

struct user_edit_form : public cppcms::form {
	cppcms::widgets::text username;
	cppcms::widgets::password password;
	cppcms::widgets::submit submit;
	cppcms::widgets::checkbox remember;

	user_edit_form();
	virtual bool validate();
};

struct signup_edit_form : public cppcms::form {
	cppcms::widgets::text username;
	cppcms::widgets::email email;
	cppcms::widgets::password password;
	cppcms::widgets::password confirm;
	cppcms::widgets::submit submit;
	cppcms::widgets::checkbox agree;
	
	signup_edit_form();
	virtual bool validate();
};

struct user : public base {
	user_edit_form form;
	std::string name;
};

struct signup : public base {
	signup_edit_form form;
	std::string name;
};

struct verify : public base {
	std::string name;
	bool verified;
};

struct reset_edit_form : public cppcms::form {
	cppcms::widgets::password password;
	cppcms::widgets::password confirm;
	cppcms::widgets::submit submit;
	reset_edit_form();
	virtual bool validate();
};

struct reset : public base {
	reset_edit_form form;
	std::string name;
	std::string id;
	bool verified;
};

struct profile : public base {
	std::string name;
	std::string email;
	bool authed;
};

struct change_password : public base {
	std::string name;
	std::string email;
	bool authed;
};

}

#endif
