/*
    __STRUCTURE OF THE ALGORITHM__
    1. Define four locations: Metro Vancouver (V), Gibsons (G), Roberts Creek (R), and Sechelt (S). These locations have the following spatial
    structure: V -- (Ferry) -- G -- (Bike path) -- R -- (Road) -- S. In other words, Vancouver is accessible from Sechelt only by passing through
    Roberts Creek and Gibsons and then taking a ferry.
    2. Populate these locations with agents according to the ratios of their populations. [note 1] The agents are randomly assigned a
    willingness to bike rating which is greater than zero, [note 2] as well as a home location.
    3. The model runs for one year (hopefully). Each day agents have a chance to take a trip to the other side of the ferry path. An agent
    who decides to go also decides on the trip length. [note 3]
    To do so, they join the ferry queue. [note 4]
    The ferry sails <number of times> per day and takes <number of agents> with it each time. Agents also have
    a balk point, i.e. a point at which they do not take the ferry if the queue is too long.
    4. In each period, record the length of the ferry queue; and the number of passengers of each type
    __NOTES__
    [note 1] At this time, agents represent families but have no notion of size. We will probably need to assume each agent is about 2
    people.
    [note 2] Will need to use some continuous distribution with positive support
    [note 3] May need to have some kind of calendar system to handle return trips
    [note 4] Really, there are two ferry queues: one for cyclists and one for motor vehicles
*/

class Agent {
public:
    char home; //possible values 'v', 'g', 'r', 's'
    float willingness_to_bike; //how willing an agent is to bike; some distribution with finite expectation and support on [0, +inf)
    float balk_point; //how many days an agent is willing to wait for the ferry
    /*
        Constructor Functions
    */
    Agent(char h, float w, float b) { //construction with an argument
        home = h;
        willingness_to_bike = w;
        balk_point = b;
    }
    Agent() { //default constructor. We should basically never be using a default agent, this is just in case we need it
        home = 'v'; //default agent lives in Vancouver
        willingness_to_bike = 0.01; //arbitrarily chosen value (see above)
        balk_point = 2.0; //arbitrarily chosen value (see above)
    }
}

int main() {
    return 0;
}
