#include <simgrid/s4u.hpp>
#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <iostream>
//Starting XBT logs
XBT_LOG_NEW_DEFAULT_CATEGORY(sample_simulator, "Messages specific for this simulator");

#define GigaByte 1000000000

//Structure that will be passed around the ring
struct Msg{
	long int size;
	std::string content;
};

//Reads a certain size of bytes from a single disk
void read_disk(std::string disk, long int size){
	simgrid::s4u::Storage* storage = simgrid::s4u::Storage::by_name(disk);
	sg_size_t read = storage->read(size);

	if (size >= GigaByte)
  		XBT_INFO("Read %llu GB from %s", read/GigaByte, disk.c_str());
  	else
  		XBT_INFO("Read %llu Bytes from %s", read, disk.c_str());

}

//Writes a certain size of bytes on a list of disks
void write_disks(std::vector<std::string> disk_vector, long int size){
	
	simgrid::s4u::Storage* storage;
	for(auto const& disk : disk_vector){
		storage = simgrid::s4u::Storage::by_name(disk);
		sg_size_t write = storage->write(size);
		if (size >= GigaByte)
  			XBT_INFO("Read %llu GB from %s", write/GigaByte, disk.c_str());
  		else
  			XBT_INFO("Read %llu Bytes from %s", write, disk.c_str());
	}

}
static void communicate(long int token_size){
  //Gets the name and position of the host
  int ring_position = std::stoi(simgrid::s4u::this_actor::get_name());
  std::string host_name = simgrid::s4u::Host::current()->get_cname();

  //Display name and position of the host
  XBT_INFO("Host: \"%s\" Ring Number: \"%u\" ", host_name.c_str() ,  ring_position);

  //Display information on the disks mounted by the current host */
  XBT_INFO("*** Storage info on \"%s\" ***", host_name.c_str());

  //Retrieve all mount points of current host */
  std::unordered_map<std::string, simgrid::s4u::Storage*> const& storage_list =
      simgrid::s4u::Host::current()->get_mounted_storages();

  //For each disk mounted on host, display disk name and mount point 
  //Get all the disk names on a vector.
  std::vector<std::string> disk_vector;
  for (auto const& disk : storage_list){
    XBT_INFO("Storage name: %s, mount name: %s \n", disk.second->get_cname(), disk.first.c_str());
    disk_vector.push_back(disk.second->get_name());
  }

  //Creates a Mailbox for the Host
  simgrid::s4u::Mailbox* mailbox= simgrid::s4u::Mailbox::by_name(simgrid::s4u::this_actor::get_name());
  
  //The neighbor host is the one that will receive the token
  int neighbor_position;
  simgrid::s4u::Mailbox* neighbor_mailbox;

  //If its the last host, his message will be send to the first
  if (ring_position == simgrid::s4u::Engine::get_instance()->get_host_count()-1){
    neighbor_position = 0;
  }
  else{
    neighbor_position = ring_position +1;
  }


  //If its the first host, it will create and send the message
  if (ring_position == 0){
    
    //Initialization of the message
    Msg token;
    token.content = "Token";
    token.size = token_size;

    //Simulates reading the token from disk
    read_disk(disk_vector[0], token.size);

    //Puts msg on the neighbor_maibox
    neighbor_mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(neighbor_position));
    XBT_INFO("Host \"%u\" sent \"%s\" to Host \"%u\" \n", ring_position, token.content.c_str(), neighbor_position);
    neighbor_mailbox->put(&token,token.size);
  

    //Waits to get the Token back
    Msg* msg = (Msg*)(mailbox->get());
    XBT_INFO("Host \"%u\" received \"%s\"", ring_position, msg->content.c_str());

    //Simulates Writing the Token on every disk connected to the host.
    write_disks(disk_vector, msg->size);
  } 
  else{//If its not the first host:
    //Gets the token
    Msg* token = (Msg*)(mailbox->get());
    XBT_INFO("Host \"%u\" received \"%s\"", ring_position, token->content.c_str());
    
    
    //Puts token on the neighbor_maibox
    neighbor_mailbox = simgrid::s4u::Mailbox::by_name(std::to_string(neighbor_position));
    XBT_INFO("Host \"%u\" sent \"%s\" to Host \"%s\" \n", ring_position, token->content.c_str() , neighbor_mailbox->get_cname());
    neighbor_mailbox->put(token,token->size);

    //Simulates writing the token on every disk connected to the host.
    write_disks(disk_vector,token->size);
    
  }

}

//ARGUMENTS: 1. platform.xml file
//			 2. number of bytes of the token (long int)
int main(int argc, char* argv[])
{
  
  //Creats Engine
  simgrid::s4u::Engine e(&argc, argv);
  //Makes sure there are 3 arguments 
  xbt_assert(argc==3, "Usage: %s platform_file.xml", argv[0]);
  e.load_platform(argv[1]);

  long int token_size = std::stol(argv[2]);
  
  XBT_INFO("Number of hosts '%zu'", e.get_host_count());

  //Creates vectors with all the hosts
  std::vector<simgrid::s4u::Host*> list = e.get_all_hosts();
  
  int ring_position=0;
  //For every host, create an Actor to be run on that host. Actor name will be his position on the ring.
  for(auto  host : list){
      simgrid::s4u::Actor::create((std::to_string(ring_position)).c_str(), host, communicate , token_size);
      ring_position++;
  }
  //Start engine and show results:
  e.run();
  std::cout << "\n";
  XBT_INFO("Simulation time: %.3f seconds.", e.get_clock());
  XBT_INFO("Equivalent time: %.3f minutes.", e.get_clock()/60);
  XBT_INFO("Equivalent time: %.3f hours.", (e.get_clock()/60)/60);
  return 0;
}

