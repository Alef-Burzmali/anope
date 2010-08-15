#include "services.h"

int Pipe::RecvInternal(char *buf, size_t sz) const
{
	static char dummy[512];
	while (read(this->Sock, &dummy, 512) == 512);
	return 0;
}

int Pipe::SendInternal(const Anope::string &) const
{
	static const char dummy = '*';
	return write(this->WritePipe, &dummy, 1);
}

Pipe::Pipe() : Socket()
{
	int fds[2];
	if (pipe2(fds, O_NONBLOCK))
		throw CoreException(Anope::string("Could not create pipe: ") + strerror(errno));

	this->Sock = fds[0];
	this->WritePipe = fds[1];
	this->IPv6 = false;
	this->Type = SOCKTYPE_CLIENT;

	SocketEngine->AddSocket(this);
}

bool Pipe::ProcessRead()
{
	this->RecvInternal(NULL, 0);
	return this->Read("");
}

bool Pipe::Read(const Anope::string &)
{
	this->OnNotify();
	return true;
}

void Pipe::Notify()
{
	this->SendInternal("");
}

void Pipe::OnNotify()
{
}

