#include <vector>
#include <random>
#include <chrono> //for seeding PRNGs
/*
    __STRUCTURE OF THE ALGORITHM__
    1. Define four locations: Metro Vancouver (V), Gibsons (G), Roberts Creek (R), and Sechelt (S). These locations have the following spatial
    structure: V -- (Ferry) -- G -- (Bike path) -- R -- (Road) -- S. In other words, Vancouver is accessible from Sechelt only by passing through
    Roberts Creek and Gibsons and then taking a ferry.
    2. Populate these locations with agents according to the ratios of their populations. [note 1] The agents are randomly assigned a
    willingness to bike: either the agent never bikes, always bikes, or only bikes if there is a path; as well as a home location. [note 2]
    3. The model runs for one year (hopefully). Each day agents have a chance to take a trip to the other side of the ferry path. An agent
    who decides to go also decides on the trip length. [note 3]
    To do so, they join the ferry queue. [note 4]
    The ferry sails <number of times> per day and takes <number of agents> with it each time. Agents also have
    a balk point, i.e. a point at which they do not take the ferry if the queue is too long.
    4. In each period, record the length of the ferry queue; and the number of passengers of each type
    __NOTES__
    [note 1] At this time, agents represent families but have no notion of size. We will probably need to assume each agent is about 2
    people.
    [note 2] The reason I am not using Thijs's suggestion to have both a bike path quality rating and give each agent a numerical willingness
    to bike and then compare the two is that this makes the numerical conclusions meaningless. Both scales can be chosen arbitrarily
    and there is no good way to measure or identify them, or even interpret them. It will be much easier if there is one, clear parameter,
    namely: the percentage of agents who are willing to bike if there is a path.
    [note 3] May need to have some kind of calendar system to handle return trips
    [note 4] Really, there are two ferry queues: one for cyclists and one for motor vehicles
*/

class Agent {
public:
    char home; //possible values 'v', 'g', 'r', 's' -- where does the agent live
    char location; //possible values 'v', 'g', 'r', 's' -- where is the agent right now? this is more efficient than having a vector of agents
    //for each location
    char will_bike; //will an agent bike to their destination? 'n' (no) 'y' (yes) or 'p' (yes if there is a path)
    float balk_point; //how many days an agent is willing to wait for the ferry
    /*
        Constructor Functions
    */
    Agent(char h, , char l, char w, float b) { //construction with an argument
        home = h;
        location = l;
        will_bike = w;
        balk_point = b;
    }
    Agent() { //default constructor. We should basically never be using a default agent, this is just in case we need it
        home = 'v'; //default agent lives in Vancouver
        location = 'v'; //default agent is currently in their home of Vancouver
        will_bike = 'n' //arbitrarily chosen value (see above)
        balk_point = 2.0; //arbitrarily chosen value (see above)
    }
    void setLocation(char new_location) {
        location = new_location;
    }
    bool isOnVacation() {
        if (location == home) {
            return false;
        }
        else return true;
    }
}

/*
    Global constants
*/
const int CARS_PER_FERRY (4); //how many cars each ferry takes
const int BIKES_PER_FERRY (4); //how many bikes each ferry takes, these numbers are currently just placeholders
const int FERRIES_PER_DAY (4);

/*
    Randomness setup
    Where do the magic numbers come from? The total estimated spend by tourists is 250*10^6 CA$. The average person
    spends $245 per trip. I assume (totally arbitrary) that 2/3 of the money is spent in the peak season (in other words that
    on average twice as many people visit during the peak season compared to the rest of the year). That just seemed
    reasonable, I couldn't get any reliable information on whether that is true or not. That yields a split of $83*10^6 in the off
    season and $166*10^6 in the peak season. Dividing that by the average spend per person yields 3.4 * 10^5 people in
    the non-peak season and 6.8*10^5 people in the peak season. The peak season is 90 days, so 7555 people/day on average,
    and for the non-peak season 1259 people/day. Note that these numbers are really the average number of trips that begin every day,
    not the total number of tourists there at a time. There are about 3*10^6 people in Vancouver, using that as the denominator yields
    the numbers below.
*/
auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
std::mt19937 randomizer;
std::poisson_distribution<> n_days(3.3); //the average trip length is 3.3 nights
std::bernoulli_distribution<> takes_trip_peak(0.0025); //chance per day the agent wants to take a trip to the sunshine coast
std::bernoulli_distribution<> takes_trip_nonpeak(0.00042); //see above for explanation of magic numbers
/*
    Helper function that will enable us to pair an Agent with an integer representing how long their vacation is
    without adding an attribute to agents or working with a computationally costly vector<Agent>.
    getTripLengths takes three arguments: First, world is a vector of all the Agents in the model. Second, trip_lengths is
    a vector<int> that stores the lengths of the vacations agents are taking. Because one cannot take a vacation of
    length 0, that corresponds to not being on vacation. Third, there is a boolean indicating whether it is the
    peak season or not, as that affects the chances of taking a trip.
*/
void getTripLengths(vector<Agent>& world, vector<int>& trip_lengths, bool is_peak) {
    if (world.size() != trip_lengths.size()) {
        throw runtime_error("The vectors world and trip_lengths must have the same size");
    } //make sure we don't do anything funky with differently sized vectors.
    for (int i (0); i < world.size(); ++i) {
        if (is_peak && !world[i].isOnVacation) { //if they're already on vacation they're not going to take another trip
            trip_lengths[i] = takes_trip_peak(randomizer) * n_days(randomizer); //because takes_trip_peak is 0 if they decide not to take a trip
        }
        else if (!world[i].isOnVacation) {
            trip_lengths[i] = takes_trip_nonpeak(randomizer) * n_days(randomizer);
        }
    } //no else statement because if they are already on vacation we don't want to fuck with that
}

int main() {
    /*
        Define ferry queues. These are all vector<int> so we can work with indices rather than
        directly with agents, which are more computationally costly
    */
    vector<int> ferry_cvg; //car, vancouver to gibsons
    vector<int> ferry_bvg; //bike, vancouver to gibsons
    vector<int> ferry_cgv; //car, gibsons to vancouver
    vector<int> ferry_bgv; //bike, gibsons to vancouver

    int t_max = 365;

    for (int t (0); t < t_max; ++t) { //main loop for the days of the year
        // logic for putting agents in ferries goes here
        for (int i (0); i < FERRIES_PER_DAY; ++i) { //loop for each ferry trip
            
        }
    }

    return 0;
}
