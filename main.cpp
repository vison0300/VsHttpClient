#include <iostream>
using namespace std;

#include "src/VsHttpClient.h"
#include "src/VsHttpFileStream.h"

static void VsHttpFileStreamTest()
{
	CVsHttpClient client;
	CVsHttpRequest request;
	CVsHttpResponse response(new CVsHttpFileStream("./www.baidu.com.html"));

	request.SetMethod("GET");
	request.SetUrl("http://www.baidu.com");

	if (client.Send(request, response))
	{
		CVsHttpByteStream s;
		response.toStream(s);

		std::vector<char> vBuf(s.GetSize() + 1);
		s.Read(&vBuf[0], vBuf.size());
		std::cout<<"recv: "<<std::endl<<&vBuf[0]<<std::endl;
	}
}

int main(int argc, char* argv[])
{
	VsHttpFileStreamTest();

	std::cout<<"\nPlease enter 'q' to exit\n";
	while ('q' != getchar());
	return 0;
}