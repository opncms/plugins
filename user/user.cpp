///////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2013-2018 Vladimir Yakunin (kpeo) <opncms@gmail.com>
//
//  The redistribution terms are provided in the COPYRIGHT.txt file
//  that must be distributed with this source code.
//
////////////////////////////////////////////////////////////////////////////

#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>

#if !(__cplusplus>=201103L)
#include <boost/assign/list_of.hpp>
#endif

#include <booster/log.h>

#include <opncms/tools.h>
#include <opncms/ioc.h>

#include <opncms/module/plugin.h>
#include <opncms/module/auth.h>
#include <opncms/module/view.h>

#include "user.h"

namespace user_sign {
	const char name[] = "Users";
	const char shortname[] = "user";
	const char slug[] = "u";
	const char version[] = "0.0.1";
	const char api_version[] = "0.0.1";
}

//TODO it's "cheaper" to use the namespace, but:
//- we need in central realization class
//- we need to create the module pointers by constructor
class user_impl
{
	friend class user;
	friend class user_rpc;

public:
	user_impl()
	:auth( &ioc::get<Auth>() ),
	data( &ioc::get<Data>() ),
	view( &ioc::get<View>() )
	{}
	
	bool send_email(const std::string& mail, const std::string& username, const std::string& hash_url, const std::string& hash, int num)
	{
		BOOSTER_LOG(debug,__FUNCTION__) << "mail(" << mail << "), username(" << username << "), hash_url(" << hash_url << ", hash(" << hash << "), num(" << num << ")";
		std::string hostfull, msg, subj, uniq, brand;
		std::vector<std::string> v;
		
		if(tools::getfullbyname(v))
			hostfull = v[0];
		else
			hostfull = tools::get_hostname();
		
		brand = view->brand();

		//Fill the message depend on selected template
		switch(num)
		{
			case 0: { //Sign up
				subj = brand + ": verify";
				uniq = ",\n\nPlease use this link to verify your account on ";
				break;
			}
			case 1: { //Reset
				subj = brand + ": reset password";
				uniq = ",\n\nPlease use this link to reset the password for your account on ";
				break;
			}
		}
		msg += "Dear ";
		msg += username;
		msg += uniq;
		msg += hostfull;
		msg += ": ";
		msg += "https://";//view->url("/");
		msg += hostfull;
		msg += hash_url;
		msg += hash;
		msg += "\nPlease do not use this link if you did not signed up on this server!\nThank you!\n\n";
		msg += "---\nSincerely yours,\n";
		msg += brand;
		
		std::string email_user = data->settings().get<std::string>("opncms.email.user", "");
		std::string email_password = data->settings().get<std::string>("opncms.email.password", "");
		bool direct = data->settings().get<bool>("opncms.email.direct", false);
		return tools::send_email(email_user, email_password, mail, subj, msg, direct);
	}
	
private:
	Auth* auth;
	Data* data;
	View* view;
};

namespace content {

user_edit_form::user_edit_form()
{
	username.name("Username");
	password.name("Password");
	submit.name("Signin");
	remember.name("Remember");

	submit.value(_("Signin"));

	add(username);
	add(password);
	add(submit);
	add(remember);

	username.non_empty();
	password.non_empty();

	username.id("username");
	password.id("password");
	remember.id("remember");
}

bool user_edit_form::validate()
{
	if(!form::validate())
		return false;

	return true;
}

signup_edit_form::signup_edit_form()
{
	username.name("Username");
	email.name("Email");
	password.name("Password");
	confirm.name("Confirm");
	submit.name("Signup");
	agree.name("Agree");

	username.message(_("username"));
	password.message(_("password"));

	submit.value(_("Signup"));

	add(username);
	add(email);
	add(password);
	add(confirm);
	add(submit);
	add(agree);

	username.non_empty();
	email.non_empty();
	password.non_empty();
	confirm.non_empty();

	username.size(USER_MIN_NAME); //minimal user name
	email.size(USER_MIN_EMAIL);
	password.size(USER_MIN_PASSWORD);
	confirm.size(USER_MIN_PASSWORD);

	username.id("username");
	email.id("email");
	password.id("password");
	confirm.id("confirm");
}

bool signup_edit_form::validate()
{
	if(!form::validate())
		return false;

	return true;
}

reset_edit_form::reset_edit_form()
{
	password.name("Password");
	confirm.name("Confirm");
	submit.name("Reset");
	submit.value(_("Reset"));

	add(password);
	add(confirm);
	add(submit);

	password.non_empty();
	confirm.non_empty();

	password.size(USER_MIN_PASSWORD);
	confirm.size(USER_MIN_PASSWORD);

	password.id("password");
	confirm.id("confirm");
}

bool reset_edit_form::validate()
{
	if(!form::validate())
		return false;
	return true;
}

}//namespace content

class user: public Plugin
{
public:

	user(cppcms::service &srv)
	: Plugin::Plugin(srv), impl()
	{
		BOOSTER_LOG(debug, __FUNCTION__) << "name=" << user_sign::name << ", shortname=" << user_sign::shortname << ", slug=" << user_sign::slug << ", version=" << user_sign::version;

		map_.push_back(user_sign::shortname);
		map_.push_back(std::string("/")+user_sign::slug+"/{1}");
		map_.push_back(std::string("/")+user_sign::slug+"(/(.*))?");
		map_.push_back("2");

		dispatcher().assign("/?", &user::display,this,0);
		mapper().assign("");

		dispatcher().assign("/signup/?",&user::signup,this);
		mapper().assign("signup","/signup");

		dispatcher().assign("/signin/?",&user::signin,this);
                mapper().assign("signin","/signin");

		dispatcher().assign("/signout/?",&user::signup,this);
		mapper().assign("signout","/signout");

		dispatcher().assign("/verify/?(\\w+)?",&user::verify,this,1);
		mapper().assign("verify","/verify/{1}");

		dispatcher().assign("/reset/?(\\w+)?",&user::reset,this,1);
		mapper().assign("reset","/reset/{1}");

		dispatcher().assign("/change/?",&user::change_password,this);
		mapper().assign("change","/change");

		dispatcher().assign("/profile/?(\\w+)?",&user::profile,this,1);
		mapper().assign("profile","/profile/{1}");
}
	~user(){
		BOOSTER_LOG(debug,__FUNCTION__);
	}

	virtual void prepare(){
		content_.name = name();
	}
	
	virtual bool post(content::signup& c)
	{
		cppcms::http::request& req = impl.view->request();

		if (req.request_method()=="POST")
		{
			BOOSTER_LOG(debug,__FUNCTION__) << "Content-Type=" << req.content_type();

			if (!req.post("Signup").empty())
			{
				BOOSTER_LOG(debug,__FUNCTION__) << "POST signup form";
				//c.form.load(ioc::get<View>().context());
				BOOSTER_LOG(debug,__FUNCTION__) << "input.email=" << req.post("email");
				BOOSTER_LOG(debug,__FUNCTION__) << "input.name=" << req.post("name");
				BOOSTER_LOG(debug,__FUNCTION__) << "input.password=" << ( req.post("password") ) ? std::string("***") : std::string();
				
				if( impl.auth->ref().exists(req.post("email")) )
				{
					c.is_alert = true;
					//c.alert_dismiss = false;
					c.alert_text = "User already exists";
					c.alert_type = "danger";
				} else {
					//generate verification code & create user
					const std::string user_email = req.post("email");
					const std::string user_password = req.post("password");
					const std::string user_name = req.post("name");
					const std::string hash = impl.auth->ref().create(user_email,user_password,user_name);
					BOOSTER_LOG(debug,__FUNCTION__) << "hash(" << hash << ")";

					if( hash.empty() )
					{
						BOOSTER_LOG(debug,__FUNCTION__) << "hash is empty or user is already exists";
						c.is_alert = true;
						c.alert_dismiss = true;
						c.alert_text = "Please, try again";
						c.alert_type = "danger";
					} else {
						//send verification hash
						impl.send_email(user_email, user_name, std::string("/")+user_sign::slug+"/verify/", hash, 0);
						BOOSTER_LOG(debug,__FUNCTION__) << "redirect to verify page";
						return true;
					}
				}

			}
			else
				BOOSTER_LOG(debug,__FUNCTION__) << "POST unknown data";
		}
		else
			BOOSTER_LOG(debug,__FUNCTION__) << " req != POST";

		return false;
	}

	virtual bool post(content::user& c)
	{
		cppcms::http::request& req = impl.view->request();

		if (req.request_method()=="POST")
		{
                        BOOSTER_LOG(debug,__FUNCTION__) << "Content-Type=" << req.content_type();

                        if (!req.post("signin").empty())
                        {
				std::string email = req.post("email");
				std::string password = req.post("password");

                                BOOSTER_LOG(debug,__FUNCTION__) << "POST signin form";
                                //c.form.load(ioc::get<View>().context());
                                BOOSTER_LOG(debug,__FUNCTION__) << "input.email=" << email;
                                BOOSTER_LOG(debug,__FUNCTION__) << "input.password=" << (password) ? std::string("***") : "";

                                if( impl.auth->ref().exists(email) )
                                {
                                        if (impl.auth->check(email,password)) {
                                                BOOSTER_LOG(debug,__FUNCTION__) << "User " << email << " authorized";
                                                if (!impl.data->session().is_exposed("email")) {
                                                        BOOSTER_LOG(error,__FUNCTION__) << "Session is not exposed!";
                                                }
						impl.view->response().set_redirect_header(std::string("/")+user_sign::slug+"/profile");
                                        }
                                        else {
                                                c.is_alert = true;
                                                c.alert_text = "Wrong email or password";
                                                c.alert_type = "danger";        
                                        }
                                } else {
                                        c.is_alert = true;
                                        c.alert_text = "Wrong email";
                                        c.alert_type = "danger";        
                                }

                        }
                        else
                                BOOSTER_LOG(debug,__FUNCTION__) << "POST unknown data";
                }
                else
                        BOOSTER_LOG(debug,__FUNCTION__) << " req != POST";

                return false;
	}

	virtual int post(content::reset& c)
	{
		cppcms::http::request& req = impl.view->request();

		if (req.request_method()=="POST")
		{
			BOOSTER_LOG(debug,__FUNCTION__) << "Content-Type=" << req.content_type();

			if (!req.post("Send").empty())
			{
				BOOSTER_LOG(debug,__FUNCTION__) << "POST send form";
				std::string reset_email = req.post("email");
				BOOSTER_LOG(debug,__FUNCTION__) << "input.email=" << reset_email;

				if( !impl.auth->ref().exists(reset_email) )
				{
					c.is_alert = true;
					//c.alert_dismiss = false;
					c.alert_text = "Incorrect email";
					c.alert_type = "danger";
					return 0;
				}
				
				//generate reset hash
				const std::string hash = impl.auth->ref().reset(reset_email);
				BOOSTER_LOG(debug,__FUNCTION__) << "hash(" << hash << ")";

				//send code
				if( hash.empty() )
				{
					BOOSTER_LOG(debug,__FUNCTION__) << "hash is empty";
					c.is_alert = true;
					c.alert_dismiss = true;
					c.alert_text = "Please, try again";
					c.alert_type = "danger";
					return 0;
				} else {
					//send hash to email
					BOOSTER_LOG(debug,__FUNCTION__) << "Send hash to email(" << reset_email << ")";
					impl.send_email(reset_email, "user", std::string("/")+user_sign::slug+"/reset/", hash, 1);
					return 1;
				}
			}
			else if(!req.post("Reset").empty())
			{
				BOOSTER_LOG(debug,__FUNCTION__) << "POST reset form";
				
				std::string reset_password = req.post("password");
				std::string reset_id = req.post("id");
				std::string reset_email = impl.auth->ref().verify(reset_id);

				BOOSTER_LOG(debug,__FUNCTION__) << "id(" << reset_id << "), email(" << reset_email << ")";
				if( reset_email.empty() || !impl.auth->ref().exists(reset_email) )
				{
					c.is_alert = true;
					//c.alert_dismiss = false;
					c.alert_text = "Incorrect email";
					c.alert_type = "danger";
					return 0;
				}
				BOOSTER_LOG(debug,__FUNCTION__) << "Generate new password";
				impl.auth->ref().password(reset_email, reset_password);
				impl.data->driver().erase("hashes",reset_id);
				return 2;
			}
			else
				BOOSTER_LOG(debug,__FUNCTION__) << "POST unknown data";
		}
		else
			BOOSTER_LOG(debug,__FUNCTION__) << " req != POST";

		return 0;
	}

	virtual void display(const std::string page)
	{
		BOOSTER_LOG(debug, __FUNCTION__) << "display" << page;
		content::user c;
		//init base content first
		impl.view->init(c);
		impl.view->post(c);

		if(!post(c))
		{
			//we get c.authed by View::post
			if(c.authed) {
				//some functions for authed users
				BOOSTER_LOG(debug, __FUNCTION__) << "User is authed";
			} else {
				BOOSTER_LOG(debug, __FUNCTION__) << "User is not authed";
				impl.view->response().set_redirect_header(std::string("/")+user_sign::slug+"/signin");
			}
			if( impl.auth->ref().block(impl.auth->id()) )
			{
				c.is_alert = true;
				c.alert_text = "Account is blocked, please reset your password";
				c.alert_type = "danger";
			}
			int utype = impl.auth->user_type();
			BOOSTER_LOG(debug,__FUNCTION__) << " user_type=" << utype;
		//std::string key="user_"+ab_.username()+"-"+tools::num2str<int>(utype)+"-"+((c.remind)?std::string("1"):std::string("0"))+":"+vb_.locale_name()+":"+upath;

			c.name = name();
		}
		render(shortname()+"_view", shortname(), c);
	}

	virtual bool post(content::profile& c)
	{
		cppcms::http::request& req = impl.view->request();

		if (req.request_method()=="POST")
		{
			 BOOSTER_LOG(debug,__FUNCTION__) << "Content-Type=" << req.content_type();
		}
		else
			BOOSTER_LOG(debug,__FUNCTION__) << " req != POST";
		return false;
	}

	virtual void profile(const std::string page)
	{
		BOOSTER_LOG(debug, __FUNCTION__);
		content::profile c;
		//init base content first
		impl.view->init(c);
		impl.view->post(c);

		//we get c.authed from View::post
		//c.authed = impl.auth->auth();
		BOOSTER_LOG(debug, __FUNCTION__) << "authed=" << c.authed;
		if(!post(c)) {
			BOOSTER_LOG(debug, __FUNCTION__) << "authed=" << c.authed;
			if(c.authed) {
				//some functions for authed users
				BOOSTER_LOG(debug, __FUNCTION__) << "User is authed";
			} else {
				BOOSTER_LOG(debug, __FUNCTION__) << "User is not authed";
			impl.view->response().set_redirect_header(std::string("/")+user_sign::slug+"/signin");
                        }
                        if( impl.auth->ref().block(impl.auth->id()) )
                        {
                                c.is_alert = true;
                                c.alert_text = "Account is blocked, please reset your password";
                                c.alert_type = "danger";
                        }
                        int utype = impl.auth->user_type();

			BOOSTER_LOG(debug,__FUNCTION__) << " user_type=" << utype;
			//std::string key="user_"+ab_.username()+"-"+tools::num2str<int>(utype)+"-"+((c.remind)?std::string("1"):std::string("0"))+":"+vb_.locale_name()+":"+upath;

			c.name = name();
		}
		render(shortname()+"_view", "profile", c);
	}

	virtual void signup()
	{
		BOOSTER_LOG(debug, __FUNCTION__);
		content::signup c;
		//init base content first
		impl.view->init(c);
		impl.view->post(c);

		if( !post(c) )
		{
			BOOSTER_LOG(debug, __FUNCTION__) << "check authed";
			//if(impl.auth->auth()) {
			if(c.authed) {
				//some functions for authed users
				BOOSTER_LOG(debug, __FUNCTION__) << "user already exists";
			} else {
				BOOSTER_LOG(debug, __FUNCTION__) << "user is not exists";
			}
			c.name = "Signup";
			render(shortname()+"_view", "signup", c);
		}
		else
			impl.view->response().set_redirect_header(std::string("/")+user_sign::slug+"/verify");
	}

        virtual void signin()
        {
                BOOSTER_LOG(debug, __FUNCTION__);
                content::user c;
                //init base content first
                impl.view->init(c);
                impl.view->post(c);

                if( !post(c) )
                {
                        BOOSTER_LOG(debug, __FUNCTION__) << "check authed";
                        //if(impl.auth->auth()) {
                        if(c.authed) {
                                //some functions for authed users
                                BOOSTER_LOG(debug, __FUNCTION__) << "user is authed";
                        } else {
                                BOOSTER_LOG(debug, __FUNCTION__) << "user is not authed";
                        }
                        c.name = "Signin";
                }
		render(shortname()+"_view", shortname(), c);
        }

	virtual void verify(const std::string id)
	{
		BOOSTER_LOG(debug, __FUNCTION__) << "id(" << id << ")";

		content::verify c;
		impl.view->init(c);
		c.name = "Verify";
		c.verified = false;
		
		if(!id.empty())
		{
			std::string email = impl.auth->ref().verify(id);
			BOOSTER_LOG(debug, __FUNCTION__) << "email(" << email << ")";
			
			if(!email.empty())
			{
				BOOSTER_LOG(debug, __FUNCTION__) << "Account verified";
				c.verified = true;
				impl.data->driver().erase("hashes",id);

				//c.is_alert = true;
				//c.alert_text = "Please Sign-in";
				//c.alert_type = "info";
				//impl.auth->password(email, newpassword)
				impl.view->response().set_redirect_header(std::string("/")+user_sign::slug+"/signin");
			} else {
				BOOSTER_LOG(debug, __FUNCTION__) << "Account not verified";
				c.is_alert = true;
				c.alert_text = "Account is not verified or expired, please check the code is correct";
				c.alert_type = "danger";
			}
		} else {
			c.is_alert = true;
			//c.alert_dismiss = false;
			c.alert_text = "Please check your email to activate the account";
			c.alert_type = "info";
		}
		render(shortname()+"_view", "verify", c);
	}

	virtual void reset(const std::string id)
	{
		BOOSTER_LOG(debug, __FUNCTION__) << "id(" << id << ")";
		content::reset c;
		impl.view->init(c);
		c.name = "Reset";
		c.verified = false;

		//if post email -> add input for hash | stay here with info
		if( post(c) == 2)
			impl.view->response().set_redirect_header("/");
			//c.is_alert = true;
			//c.alert_dismiss = false;
			//c.alert_text = "Please check your email to activate the account";
			//c.alert_type = "danger";

		//if have id -> verify(get email)
		//	if verified -> show form to change password
		//	else -> show alert
		//if have no id -> new form with info
		if(!id.empty())
		{
			BOOSTER_LOG(debug, __FUNCTION__) << "Hash is exists, verifying.";
			std::string email = impl.auth->ref().verify(id);
			
			if(!email.empty())
			{
				BOOSTER_LOG(debug, __FUNCTION__) << "Account verified.";
				c.verified = true;
				c.is_alert = true;
				c.alert_text = "Please input new password";
				c.alert_type = "info";
				//impl.auth->password(email, newpassword)
				//impl.view->response().set_redirect_header("/u/");
				c.id = id;
				//impl.view->response().set_redirect_header("/u/");
			} else {
				BOOSTER_LOG(debug, __FUNCTION__) << "Account is not verified";
				c.verified = false;
				c.is_alert = true;
				c.alert_text = "Account is not verified, please check the code is correct";
				c.alert_type = "danger";
			}
		} else {
			c.is_alert = true;
			//c.alert_dismiss = false;
			c.alert_text = "Please input your email to reset the password";
			c.alert_type = "info";
			
		}
		render(shortname()+"_view", "reset", c);
	}

	virtual bool post(content::change_password& c)
	{
		cppcms::http::request& req = impl.view->request();

		if (req.request_method()=="POST")
		{
			BOOSTER_LOG(debug,__FUNCTION__) << "Content-Type=" << req.content_type();
			
			if(!req.post("Change").empty())
			{
				BOOSTER_LOG(debug,__FUNCTION__) << "POST change form";

				std::string oldpassword = req.post("oldpassword");
				std::string newpassword = req.post("newpassword");
				std::string email = req.post("email");

				BOOSTER_LOG(debug,__FUNCTION__) << "email(" << email << ")";

				if( email.empty() || oldpassword.empty() || newpassword.empty() )
				{
					c.is_alert = true;
					//c.alert_dismiss = false;
					c.alert_text = "Please input correct values";
					c.alert_type = "danger";
					return false;
				}
				BOOSTER_LOG(debug,__FUNCTION__) << "Generate new password";
				if( impl.auth->ref().password(email, oldpassword, newpassword) )
					return true;
				else
				{
					c.is_alert = true;
					//c.alert_dismiss = false;
					c.alert_text = "Please check your input";
					c.alert_type = "danger";
					return false;
				}
			}
			else
				BOOSTER_LOG(debug,__FUNCTION__) << "POST unknown data";
		}
		else
			BOOSTER_LOG(debug,__FUNCTION__) << " req != POST";

		//c.authed = ioc::get<Auth>().auth();
		//c.remind = ioc::get<Auth>().remind();
		return false;
	}

	virtual void change_password()
	{
		content::change_password c;
		impl.view->init(c);
		impl.view->post(c);
		//c.authed = impl.auth->auth();
		c.email = impl.auth->id();

		//if post email -> add input for hash | stay here with info
		if( post(c) && !c.is_alert)
		{
			c.is_alert = true;
			c.alert_text = "Password has changed";
			c.alert_type = "info";
			impl.view->response().set_redirect_header(std::string("/")+user_sign::slug+"/profile");
		}
		else
			render(shortname()+"_view", "change_password", c);
	}

	void signout()
	{
		BOOSTER_LOG(debug,__FUNCTION__);
		impl.auth->deauth();

		if ( !impl.view->request().http_referer().empty() ) {
			impl.view->response().set_redirect_header(impl.view->request().http_referer());
		} else {
			BOOSTER_LOG(debug,__FUNCTION__) << " same referer: " << impl.view->request().http_referer();
			impl.view->response().set_redirect_header("/");
		}
	}

	//Extend base view
	virtual bool is_css(){ return true; }
	virtual bool is_js_head(){ return true; }
	virtual bool is_js_foot(){ return true; }
	virtual bool is_menu(){ return true; }
	virtual bool is_left(){ return false; }
	virtual bool is_right(){ return false; }
	virtual bool is_top(){ return false; }
	virtual bool is_bottom(){ return false; }
	virtual bool is_content(){ return false; }
	virtual cppcms::base_content& html_css() { if(is_css()) impl.view->init(content_); return content_; }
	virtual cppcms::base_content& html_js_head() { if(is_js_head()) impl.view->init(content_); return content_; }
	virtual cppcms::base_content& html_js_foot() { if(is_js_foot()) impl.view->init(content_); return content_; }
	virtual cppcms::base_content& html_menu() { return content_; }
	virtual cppcms::base_content& html_left() { return content_; }
	virtual cppcms::base_content& html_right() { return content_; }
	virtual cppcms::base_content& html_top() { return content_; }
	virtual cppcms::base_content& html_bottom() { return content_; }
	virtual cppcms::base_content& html_content() { return content_; }

	virtual cppcms::application& get(){
		return *this;
	}

	virtual std::string skin(){
		return std::string(user_sign::shortname) + "_view";
	}
	virtual std::string view(const std::string& s){
		return std::string(user_sign::shortname) + "_" + s;
	}

	virtual std::string name() const {
		return user_sign::name;
	}
	virtual std::string shortname() const {
		return user_sign::shortname;
	}
	virtual std::string slug() const {
		return user_sign::slug;
	}
	virtual std::string version() const {
		return user_sign::version;
	}
	virtual tools::vec_str& map() {
		return map_;
	}
private:
	user_impl impl;
	tools::vec_str map_;
	content::user content_;
	std::string salt_;
};

class user_rpc : public PluginRpc
{
public:
	user_rpc(cppcms::service &srv)
	: PluginRpc::PluginRpc(srv), impl()
	{
		BOOSTER_LOG(debug, __FUNCTION__);

		map_.push_back(std::string(user_sign::shortname)+"_rpc");
		map_.push_back(std::string("/")+std::string(user_sign::slug)+"_rpc/{1}");
		map_.push_back(std::string("/")+std::string(user_sign::slug)+"_rpc(/(.*))?");
		map_.push_back("2");

		bind("system.listMethods",cppcms::rpc::json_method(&user_rpc::methods,this),method_role);
		bind("signin",cppcms::rpc::json_method(&user_rpc::signin,this),method_role);
		bind("signup",cppcms::rpc::json_method(&user_rpc::signup,this),method_role);
		bind("signout",cppcms::rpc::json_method(&user_rpc::signout,this),method_role);
		bind("reset",cppcms::rpc::json_method(&user_rpc::reset,this),method_role);
		//bind("salt",cppcms::rpc::json_method(&user_rpc::salt,this),method_role);
		
#if __cplusplus>=201103L
		methods_ = { "system.listMethods", "signin", "signup", "signout", "reset" };
#else
		methods_ = boost::assign::list_of ("system.listMethods")("signin")("signup")("signout")("reset");
		BOOST_ASSERT( methods_.size() == 5 );
#endif
	}

	virtual void methods()
	{
		BOOSTER_LOG(debug, __FUNCTION__);
		return_result(methods_);
	}

	virtual void signin(const std::string& id, const std::string& password)
	{
		BOOSTER_LOG(debug, __FUNCTION__);
		bool rpc_result = false;
		booster::regex id_regex("^[a-zA-Z0-9_.@]+$");
		//booster::regex password_regex("^([a-zA-Z0-9@*#~!@#$%^&*()_+]{8,64})$");
		booster::smatch res;

		//std::string id_regex = settings().get<std::string>(std::string(OPNCMS_CONF)+db_type+".driver",OPNCMS_DATA_DRIVER);
	
		if (booster::regex_match(id,res,id_regex))
		{
			BOOSTER_LOG(debug,__FUNCTION__) << "Input validated, processing authorize";
			std::string uname;
			impl.data->session().load();

			if (impl.auth->check(id,password))
			{
				BOOSTER_LOG(debug,__FUNCTION__) << "User " << id << " authorized";

//				impl.data->session()["username"] = impl.auth->username(); //it may changes in auth
//				impl.data->session().expose("username");

				if (!impl.data->session().is_exposed("id")) {
					BOOSTER_LOG(error,__FUNCTION__) << "Session is not exposed!";
				}
				//ioc::get<Auth>().ref().create(ioc::get<Auth>().username(), ioc::get<Auth>().local());
				rpc_result = true;
			} else {
				BOOSTER_LOG(debug,__FUNCTION__) << "Incorrect login, id(" << id << ")";
				return_error("Incorrect login");
			}
		} else {
			BOOSTER_LOG(debug,__FUNCTION__) << "User " << id << " not validated. Please check the syntax";
			return_error("Can't validate user. Please check your inputs");
		}
	
		if(rpc_result)
			return_result("ok");
	}

	virtual void signup(const std::string& email, const std::string& password, const std::string& name)
	{
		BOOSTER_LOG(debug, __FUNCTION__);
		
		if( impl.auth->ref().exists(email) )
			return_error("User already exists");
		
		//generate verification code
		//create user
		std::string hash = impl.auth->ref().create(email,password,name);
		//send code
		
		//send hash to email
		impl.send_email(email, name, std::string("/")+user_sign::slug+"/verify/", hash, 0);

		return_result("ok");
	}

	virtual void signout()
	{
		BOOSTER_LOG(debug, __FUNCTION__);
		std::string key = "user_" + 
			impl.auth->id() + "-" +
			fmt::format("{}", impl.auth->user_type()) + "-" + 
			( ( impl.auth->remind() ) ? std::string("1") : std::string("0") ) + ":" + 
			impl.view->locale_name() + ":/";
		impl.data->cache().rise(key);

		key = "user_" + impl.auth->id() + "-" +
			fmt::format("{}", impl.auth->user_type()) + "-" +
			( ( impl.auth->remind() ) ? std::string("1") : std::string("0") ) + ":" +
			impl.view->locale_name() + ":home";
		impl.data->cache().rise(key);
		
		impl.auth->deauth();
		//redirect to current page or home page
		
		return_result("ok");
	}

	virtual void reset(const std::string& email)
	{
		BOOSTER_LOG(debug, __FUNCTION__);
		
		if( !impl.auth->ref().exists(email) )
			return_error("User is not exists");
		
		//generate reset hash
		std::string hash = impl.auth->ref().reset(email);
		BOOSTER_LOG(debug,__FUNCTION__) << "hash(" << hash << ")";
		//send code

		if( hash.empty() )
			return_error("Please, try again");

		//send hash to email
		impl.send_email(email, "user", std::string("/")+user_sign::slug+"/reset/", hash, 1);

		return_result("ok");
	}

	virtual tools::vec_str& map(){
		return map_;
	}

private:
	user_impl impl;
	tools::vec_str map_;
	cppcms::json::array methods_;
};

extern "C" void plugin()
{
	cppcms::service &s = ioc::get<View>().service();
	if(ioc::get<Plug>().get(user_sign::shortname) == NULL && ioc::get<Plug>().get_rpc(user_sign::shortname) == NULL)
	{
		BOOSTER_LOG(debug, __FUNCTION__) << "Try to add the plugin " << user_sign::name;
		ioc::get<Plug>().add(user_sign::shortname,new user(s),new user_rpc(s),user_sign::api_version);
		BOOSTER_LOG(debug, __FUNCTION__) << "Add plugin's menu items";
		ioc::get<View>().menu_add("sidebar",user_sign::name, std::string("/")+user_sign::slug);
	}
}
