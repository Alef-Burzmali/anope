/* NickServ core functions
 *
 * (C) 2003-2010 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

/*************************************************************************/

#include "module.h"

class CommandNSIdentify : public Command
{
 public:
	CommandNSIdentify(const Anope::string &cname) : Command(cname, 1, 2)
	{
		this->SetFlag(CFLAG_ALLOW_UNREGISTERED);
	}

	CommandReturn Execute(User *u, const std::vector<Anope::string> &params)
	{
		Anope::string nick = params.size() == 2 ? params[0] : u->nick;
		Anope::string pass = params[params.size() - 1];

		NickAlias *na = findnick(nick), *this_na = findnick(u->nick);
		if (!na)
		{
			NickRequest *nr = findrequestnick(nick);
			if (nr)
				notice_lang(Config->s_NickServ, u, NICK_IS_PREREG);
			else
				notice_lang(Config->s_NickServ, u, NICK_NOT_REGISTERED);
		}
		else if (na->HasFlag(NS_FORBIDDEN))
			notice_lang(Config->s_NickServ, u, NICK_X_FORBIDDEN, na->nick.c_str());
		else if (na->nc->HasFlag(NI_SUSPENDED))
			notice_lang(Config->s_NickServ, u, NICK_X_SUSPENDED, na->nick.c_str());
		/* You can now identify for other nicks without logging out first,
		 * however you can not identify again for the group you're already
		 * identified as
		 */
		else if (u->Account() && u->Account() == na->nc)
			notice_lang(Config->s_NickServ, u, NICK_ALREADY_IDENTIFIED);
		else
		{
			int res = enc_check_password(pass, na->nc->pass);
			if (!res)
			{
				Log(LOG_COMMAND, u, this) << "and failed to identify";
				notice_lang(Config->s_NickServ, u, PASSWORD_INCORRECT);
				if (bad_password(u))
					return MOD_STOP;
			}
			else if (res == -1)
				notice_lang(Config->s_NickServ, u, NICK_IDENTIFY_FAILED);
			else
			{
				if (u->IsIdentified())
					Log(LOG_COMMAND, "nickserv/identify") << "to log out of account " << u->Account()->display;

				na->last_realname = u->realname;
				na->last_seen = Anope::CurTime;

				u->Login(na->nc);
				ircdproto->SendAccountLogin(u, u->Account());
				ircdproto->SetAutoIdentificationToken(u);

				if (this_na && this_na->nc == na->nc)
					u->SetMode(NickServ, UMODE_REGISTERED);

				u->UpdateHost();

				FOREACH_MOD(I_OnNickIdentify, OnNickIdentify(u));

				Log(LOG_COMMAND, u, this) << "and identified for account " << u->Account()->display;
				notice_lang(Config->s_NickServ, u, NICK_IDENTIFY_SUCCEEDED);
				if (ircd->vhost)
					do_on_id(u);
				if (Config->NSModeOnID)
					do_setmodes(u);

				if (Config->NSForceEmail && u->Account() && u->Account()->email.empty())
				{
					notice_lang(Config->s_NickServ, u, NICK_IDENTIFY_EMAIL_REQUIRED);
					notice_help(Config->s_NickServ, u, NICK_IDENTIFY_EMAIL_HOWTO);
				}

				if (u->IsIdentified())
					check_memos(u);
			}
		}
		return MOD_CONT;
	}

	bool OnHelp(User *u, const Anope::string &subcommand)
	{
		notice_help(Config->s_NickServ, u, NICK_HELP_IDENTIFY);
		return true;
	}

	void OnSyntaxError(User *u, const Anope::string &subcommand)
	{
		syntax_error(Config->s_NickServ, u, "IDENTIFY", NICK_IDENTIFY_SYNTAX);
	}

	void OnServHelp(User *u)
	{
		if (this->name.equals_ci("IDENTIFY"))
			notice_lang(Config->s_NickServ, u, NICK_HELP_CMD_IDENTIFY);
	}
};

class NSIdentify : public Module
{
	CommandNSIdentify commandnsidentify, commandnsid;

 public:
	NSIdentify(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator), commandnsidentify("IDENTIFY"), commandnsid("ID")
	{
		this->SetAuthor("Anope");
		this->SetType(CORE);

		this->AddCommand(NickServ, &commandnsidentify);
		this->AddCommand(NickServ, &commandnsid);
	}
};

MODULE_INIT(NSIdentify)