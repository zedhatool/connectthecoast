#include <vector>
#include <deque>
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>
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
    [note 1] At this time, agents represent families but have no notion of size. Each agent represents MODEL_SCALE people, where
    MODEL_SCALE is a constant float defined below with the rest of the global constants.
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
    /*
        Constructor Functions
    */
    Agent(char h, char l, char w) { //construction with an argument
        home = h;
        location = l;
        will_bike = w;
    }
    Agent() { //default constructor. We should basically never be using a default agent, this is just in case we need it
        home = 'v'; //default agent lives in Vancouver
        location = 'v'; //default agent is currently in their home of Vancouver
        will_bike = 'n'; //arbitrarily chosen value (see above)
    }
    void setLocation(char new_location) {
        location = new_location;
    }
    char getLocation() const{
        return location;
    }
    char getHome() const{
        return home;
    }
    void setBike(char new_willingness) {
        will_bike = new_willingness;
    }
    char willBike() const{
        return will_bike;
    }
    bool isOnVacation() {
        if (location == home) {
            return false;
        }
        else return true;
    }
    Agent& operator= (const Agent& other) {
        home = getHome();
        location = getLocation();
        will_bike = willBike();
        return *this;
    }
};

/*
    Global constants
*/
const float MODEL_SCALE (2.7); //the number of people per agent in the model
const int CARS_PER_FERRY (311 / MODEL_SCALE); //how many cars each ferry takes
const int BIKES_PER_FERRY (1000 / MODEL_SCALE); //how many bikes each ferry takes, these numbers are currently just placeholders
const int FERRIES_PER_DAY (4);
const double POPULATION_VANCOUVER (2.64e6); //scientific notation, 2.64e6 = 2.64*10^6 = 2.64 million
const double POPULATION_SECHELT (1.0e4); //i want the extra precision of a double for these big numbers.
const double POPULATION_GIBSONS (5.0e3);
const double POPULATION_ROBERTSCREEK (3.0e3);
const double TOTAL_POPULATION = POPULATION_SECHELT + POPULATION_GIBSONS + POPULATION_ROBERTSCREEK;
char bike_path ('u'); //4 values: u for unset, n for "none," r for "only to robert's creek," and s for "all the way to sechelt"

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
    that during the peak season the individual propensity to take a trip (takes_trip_peak) is 0.0025; during the off season that number
    is 0.00042 (takes_trip_nonpeak). We then multiply by the model scale to make sure we are working with agents rather than
    people.
*/
auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
std::mt19937 randomizer;
std::poisson_distribution<> n_days(3.3); //the average trip length is 3.3 nights
std::bernoulli_distribution takes_trip_peak(0.0025 * MODEL_SCALE); //chance per day the agent wants to take a trip to the sunshine coast
std::bernoulli_distribution takes_trip_nonpeak(0.00042 * MODEL_SCALE); //see above for explanation of magic numbers
/*
    Helper function that will enable us to pair an Agent with an integer representing how long their vacation is
    without adding an attribute to agents or working with a computationally costly vector<Agent>.
    getTripLengths takes three arguments: First, world is a vector of all the Agents in the model. Second, trip_lengths is
    a vector<int> that stores the lengths of the vacations agents are taking. Because one cannot take a vacation of
    length 0, that corresponds to not being on vacation. Third, there is a boolean indicating whether it is the
    peak season or not, as that affects the chances of taking a trip.
*/
void getTripLengths(std::vector<Agent>& world, std::vector<int>& trip_lengths, bool is_peak) {
    if (world.size() != trip_lengths.size()) {
        throw std::runtime_error("The vectors world and trip_lengths must have the same size");
    } //make sure we don't do anything funky with differently sized vectors.
    for (int i (0); i < world.size(); ++i) {
        if (is_peak && !world[i].isOnVacation()) { //if they're already on vacation they're not going to take another trip
            trip_lengths[i] = takes_trip_peak(randomizer) * n_days(randomizer); //because takes_trip_peak is 0 if they decide not to take a trip
        }
        else if (!world[i].isOnVacation()) {
            trip_lengths[i] = takes_trip_nonpeak(randomizer) * n_days(randomizer);
        }
    } //no else statement because if they are already on vacation we don't want to fuck with that
}
/*
    Where will each Agent go on vacation?
*/
//std::uniform_int_distribution<> dis(0, 2);
std::vector<int> subintervals = {0, 1, 2, 3};
std::vector<double> weights = {POPULATION_GIBSONS / TOTAL_POPULATION, POPULATION_ROBERTSCREEK / TOTAL_POPULATION,
    POPULATION_SECHELT / TOTAL_POPULATION};
std::piecewise_constant_distribution<> d(subintervals.begin(), subintervals.end(), weights.begin());
void getDestinations(std::vector<Agent>& world, std::vector<char>& destinations) {
    for (int i (0); i < world.size(); ++i) {
        if (world[i].getHome() == 'v') {
            if (d(randomizer) < 1) destinations[i] = 'g';
            else if (d(randomizer) >= 1 && d(randomizer) < 2) destinations[i] = 'r';
            else destinations[i] = 's';
        }
        else {
            destinations[i] = 'v';
        }
    }
}

int main() {
    /*
        Initialize agents. We will use a vector<Agent> but try not to do much with it because it will be computationally intensive.
        First we define a vector, then we add items to it in accordance with the population of the areas in question:
        Sechelt, Gibsons, Roberts Creek, and Metro Vancouver.
        The population of Metro Vancouver is ~2.64 million people. The population of Sechelt is ~10 thousand, Gibsons about 5 thousand,
        and Roberts Creek about 3 thousand.
        All agents for now are non-bikers, later we will randomly assign some of the agents to be bikers.
    */
    std::vector<Agent> british_columbia;
    double j (0.0); //this avoids ad-hoc casting to int and a bunch of control flow
    while (j < POPULATION_VANCOUVER) {
        Agent agent('v', 'v', 'n'); //this is kinda ugly but it works. open to suggestions for making it cleaner
        british_columbia.push_back(agent);
        j += 1;
    }
    j = 0.0;
    while (j < POPULATION_SECHELT) {
        Agent agent('s', 's', 'n');
        british_columbia.push_back(agent);
        j += 1;
    }
    j = 0.0;
    while (j < POPULATION_GIBSONS) {
        Agent agent('g', 'g', 'n');
        british_columbia.push_back(agent);
        j += 1;
    }
    j = 0.0;
    while (j < POPULATION_ROBERTSCREEK) {
        Agent agent('r', 'r', 'n');
        british_columbia.push_back(agent);
        j += 1;
    }
    j = 0.0; //so we can use it again later. Efficiency!

    std::vector<int> trip_lengths (british_columbia.size()); //declare a vector of trip lenghts the same size as the british columbia vector
    std::vector<char> destinations (british_columbia.size());
    bool peak_season (false);
    /*
        Now some agents are willing to bike, this is a user-defined variable. I assume 1% of people
        are die-hard cyclists willing to bike from Vancouver to the Sunshine Coast even absent a bike lane.
        That is a totally arbitrary choice, and there is code to make it a user-defined variable commented out
        below. It should not affect the model too much.

        Once we know p_bike_if_lane we randomly assign some of the agents to be bike lane cyclists and some to be
        die-hard cyclists.
    */
    float p_bike_if_lane;
    float p_always_bike (0.01);
    int n_iterations;

    std::cout << "How long does the bike path extend? Enter 'n' for no path, 'r' for Roberts Creek, and 's' for Sechelt. (The input is case-sensitive.)" <<std::endl;
    std::cin >> bike_path;
    while (bike_path != 'n' && bike_path != 'r' && bike_path != 's') {
        std::cout << "Enter 'n' for no path, 'r' for Roberts Creek, and 's' for Sechelt. The input is case-sensitive." << std::endl;
        std::cin >> bike_path;
    }
    std::cout << "What proportion of people are willing to bike, if there is an available lane?" << std::endl;
    std::cout << "Enter the proportion as a decimal:" << std::endl;
    std::cin >> p_bike_if_lane;
    while (p_bike_if_lane > 1.0 || p_bike_if_lane < 0.0) {
        std::cout << "The proportion of people willing to bike must be a number between 0 and 1, expressed as a decimal." << std::endl;
        std::cout << "Enter the proportion again:" << std::endl;
        std::cin >> p_bike_if_lane; //janky input handling
    }
    std::cout << "How many times to run the model, for later averaging purposes? (Must be an integer, max 100)." << std::endl;
    std::cin >> n_iterations;
    while (n_iterations < 1 || n_iterations > 100) {
        std::cout << "The number of iterations must be an integer between 1 and 100." << std::endl;
        std::cin >> n_iterations;
    }

    /*
        If needed we can have p_always_bike be user-defined as well, just by using the following code.
    */
    //std::cout << "What proportion of people are die-hard cyclists, who bike everywhere, even if there is no available lane?" << std::endl;
    //std::cout << "Enter the proportion as a decimal:" << std::endl;
    //std::cin >> p_always_bike;
    //while (p_always_bike > 1.0 || p_always_bike < 0.0) {
    //    std::cout << "The proportion of die-hard cyclists must be a number between 0 and 1, expressed as a decimal." << std::endl;
    //    std::cout << "Enter the proportion again:" << std::endl;
    //    std::cin >> p_always_bike;
    //}
    std::bernoulli_distribution p_lane_biker (p_bike_if_lane); //i am bad at thinking of variable names
    std::bernoulli_distribution  p_die_hard (p_always_bike);
    bool coin;
    for (int i (0); i < british_columbia.size(); ++i) {
        coin = p_die_hard(randomizer);
        if (coin) {
            british_columbia[i].setBike('y');
        }
        else {
            coin = p_lane_biker(randomizer);
            if (coin) {
                british_columbia[i].setBike('p');
            }
        }
    }
    /*
        For ease of analysis and preservation, output the data to a file
    */
    std::ofstream outf("data.csv");

    if (!outf) {
        std::cerr << "The file output failed." << std::endl;
        return 1;
    }

    /*
        Define the file data structure
    */
    if (n_iterations == 1) outf << "Day,Car Trips to Coast,Bike Trips to Coast" << std::endl; //we can add tracking for people leaving the coast as well
    else if (n_iterations > 1) {
        outf << "Day";
        for (int i (0); i < n_iterations; ++i) {
            outf << ",Car Trips " << i << "," << "Bike Trips " << i;
        }
        outf << std::endl;
    }

    int car_trips_to_coast (0);
    int bike_trips_to_coast (0);
    int car_trips_to_van (0);
    int bike_trips_to_van (0);

    /*
        Define ferry queues. These are all deque<int> so we can work with indices rather than
        directly with agents, which are more computationally costly; and so we have easy insertion
        and deletion at both ends of the queue
    */
    std::deque<int> ferry_cvg; //car, vancouver to gibsons
    std::deque<int> ferry_bvg; //bike, vancouver to gibsons
    std::deque<int> ferry_cgv; //car, gibsons to vancouver
    std::deque<int> ferry_bgv; //bike, gibsons to vancouver

    int passengers_cvg (0);
    int passengers_bgv (0);
    int passengers_cgv (0);
    int passengers_bvg (0);

    int t_max = 365;
    getDestinations(british_columbia, destinations);

    /*
        To make it easy to do multiple iterations, store the data in three vectors and then output the vectors to a file.
        Since we are going to iterate through it in order of entries at the end, the best idea is to think about it like a 2d
        array, where the rows are days and the columns are iterations. In that case the data series of iteration n
        on day t would be output[t][n]. But this is a 1d vector, so we need to make a translation. We know that
        the width (number of columns) of that 2d array would be n_iterations. Therefore, we have
        output[t][n] = one_dimensional_output[t + (int) n / n_iterations]
    */
    std::vector<int> output_car_trips (n_iterations * 365);
    std::vector<int> output_bike_trips (n_iterations * 365);

    for (int n (0); n < n_iterations; ++n) {
        for (int t (0); t < t_max; ++t) { //main loop for the days of the year
            std::cout << "This is day " <<  t << std::endl;
            // outf << t << ","; old output
            if (t >= 151 && t <= 243) peak_season = true;
            else peak_season = false;
            // logic for putting agents in ferries goes here
            getTripLengths(british_columbia, trip_lengths, peak_season);
            for (int k (0); k < british_columbia.size(); ++k) {
                if (!british_columbia[k].isOnVacation() && trip_lengths[k] > 0) { //go on vacation
                    /*
                        The code below checks a bunch of possible cases. I don't know a good way of simplifying it, and
                        this is definitely going to be a computational bottleneck. This is what we call "evil physics student code."
                    */
                    if (destinations[k] != 'v' && british_columbia[k].willBike() == 'y') {
                        ferry_bvg.push_back(k);
                    }
                    else if (destinations[k] != 'v' && british_columbia[k].willBike() == 'n') {
                        ferry_cvg.push_back(k);
                    }
                    else if (destinations[k] != 'v' && british_columbia[k].willBike() == 'p' && (destinations[k] == bike_path)) {
                        ferry_bvg.push_back(k);
                    }
                    else if (destinations[k] != 'v' && british_columbia[k].willBike() == 'p' && (destinations[k] != bike_path)) {
                        ferry_cvg.push_back(k);
                    }
                    else if (destinations[k] == 'v' && british_columbia[k].willBike() == 'n') {
                        ferry_cgv.push_back(k);
                    }
                    else if (destinations[k] == 'v' && british_columbia[k].willBike() == 'y') {
                        ferry_bgv.push_back(k);
                    }
                    else if (destinations[k] == 'v' && british_columbia[k].willBike() == 'p' && british_columbia[k].getHome() == 'g') {
                        ferry_bgv.push_back(k);
                    }
                    else if (destinations[k] == 'v' && british_columbia[k].willBike() == 'p' &&
                        (british_columbia[k].getHome() == bike_path || (bike_path == 's' && british_columbia[k].getHome() == 'r'))) {
                        ferry_bgv.push_back(k);
                    }
                    else {
                        ferry_cgv.push_back(k);
                    }
                }
                else if (british_columbia[k].isOnVacation() && trip_lengths[k] <= 0) { //return home
                    if (british_columbia[k].getLocation() == 'v' && british_columbia[k].willBike() == 'y') {
                        ferry_bvg.push_back(k);
                    }
                    else if (british_columbia[k].getLocation() == 'v' && british_columbia[k].willBike() == 'n') {
                        ferry_cvg.push_back(k);
                    }
                    else if (british_columbia[k].getLocation() == 'v' && british_columbia[k].willBike() == 'p'
                        && british_columbia[k].getHome() == bike_path) {
                            ferry_bvg.push_back(k); //decide to bike
                    }
                    else if (british_columbia[k].getLocation() == 'v' && british_columbia[k].willBike() == 'p'
                        && british_columbia[k].getHome() != bike_path) {
                            ferry_cvg.push_back(k); //decide not to bike
                        }
                    else if (british_columbia[k].getLocation() != 'v' && british_columbia[k].willBike() == 'y') {
                        ferry_bgv.push_back(k);
                    }
                    else if (british_columbia[k].getLocation() != 'v' && british_columbia[k].willBike() == 'n') {
                        ferry_cgv.push_back(k);
                    }
                    else if (british_columbia[k].getLocation() != 'v' && british_columbia[k].willBike() == 'p'
                        && bike_path == 'n') {
                            ferry_cgv.push_back(k);
                        }
                    else if (british_columbia[k].getLocation() == 's' && british_columbia[k].willBike() == 'p'
                        && bike_path == 's') {
                            ferry_bgv.push_back(k);
                        }
                    else if (british_columbia[k].getLocation() == 's' && british_columbia[k].willBike() == 'p'
                        && bike_path != 's') {
                            ferry_cgv.push_back(k);
                        }
                    else if ((british_columbia[k].getLocation() == 'r' || british_columbia[k].getLocation() == 'g')
                        && british_columbia[k].willBike() == 'p' && bike_path != 'n') {
                            ferry_bgv.push_back(k);
                        }
                    else { //hopefully we do not need this
                        std::cout << "You're missing a case. Failed to classify the agent with the following characteristics:" <<std::endl;
                        std::cout << "Location: " << british_columbia[k].getLocation() << std::endl;
                        std::cout << "Willingness to Bike: " << british_columbia[k].willBike() << std::endl;
                        std::cout << "Home: " << british_columbia[k].getHome() << std::endl;
                    }
                }
                else if (british_columbia[k].isOnVacation() && trip_lengths[k] > 0) {
                    --trip_lengths[k]; //one vacation day over
                }
            }
            /*
                This is ugly and slow but deals with the problem of the ferry being underbooked. I am sure there is a faster and better way
                of doing this, but I do not know what it is right now.
            */
            for (int i (0); i < FERRIES_PER_DAY; ++i) {
                passengers_bvg = std::min(BIKES_PER_FERRY, (int) ferry_bvg.size());
                passengers_bgv = std::min(BIKES_PER_FERRY, (int) ferry_bgv.size());
                passengers_cgv = std::min(CARS_PER_FERRY, (int) ferry_cgv.size());
                passengers_cvg = std::min(CARS_PER_FERRY, (int) ferry_cvg.size());

                for (int m (0); m < passengers_bvg; ++m) british_columbia[ferry_bvg[m]].setLocation(destinations[ferry_bvg[m]]);
                ferry_bvg.erase(ferry_bvg.begin(), ferry_bvg.begin() + passengers_bvg);
                bike_trips_to_coast += passengers_bvg;
                for (int n (0); n < passengers_bgv; ++n) british_columbia[ferry_bgv[n]].setLocation(destinations[ferry_bgv[n]]);
                ferry_bgv.erase(ferry_bgv.begin(), ferry_bgv.begin() + passengers_bgv);
                bike_trips_to_van += passengers_bgv;
                for (int h (0); h < passengers_cgv; ++h) british_columbia[ferry_cgv[h]].setLocation(destinations[ferry_cgv[h]]);
                ferry_cgv.erase(ferry_cgv.begin(), ferry_cgv.begin() + passengers_cgv);
                car_trips_to_van += passengers_cgv;
                for (int g (0); g < passengers_cvg; ++g) british_columbia[ferry_cvg[g]].setLocation(destinations[ferry_cvg[g]]);
                ferry_cvg.erase(ferry_cvg.begin(), ferry_cvg.begin() + passengers_cvg);
                car_trips_to_coast += passengers_cvg;
            }
            //outf << car_trips_to_coast << "," << bike_trips_to_coast << std::endl; //old output method
            output_car_trips[n * n_iterations + t] = car_trips_to_coast;
            output_bike_trips[n * n_iterations + t] = bike_trips_to_coast;
            //new output method writes to vector first then vector to file, using Row-major order
        }
    }
    /*
        Now write the vector output to the output file
    */
    for (int t (0); t < t_max; ++t) {
        outf << t;
        for (int n (0); n < n_iterations; ++n) {
            outf << "," << output_car_trips[n * n_iterations + t] << "," << output_bike_trips[n * n_iterations + t];
        }
        outf << std::endl;
    }
    return 0;
}
