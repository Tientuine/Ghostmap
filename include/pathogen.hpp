/*
    Possible Approaches:
    - [*] Implicit Pathogen
        - focus only on the state of the host
        - SEIRD model - Susceptible/Exposed/Infected/Recovered/Deceased
    - [ ] Implicit Host
        - focus only on the presence/state of the pathogen
        - APEXR model - Absent/Present/Established/eXpired/Rejected
    - [ ] Explicit
        - consider host as a container for pathogen
        - either SEIRD or blend of the two
*/
#ifndef HPP_PATHOGEN
#define HPP_PATHOGEN

#include <random>
#include <tuple>

// 

/// <summary>
/// Integer tuple representation - 16bits each (storage upper-bound = 6*N Bytes)
/// </summary>
/// <remarks>
/// State (1st elem):
/// - susceptible = 0
/// - exposed = 1
/// - infected = 2
/// - resolved = 3
/// - recovered = 4
/// - deceased = 5
/// Days remaining (2nd elem):
/// - incubation = [1,kE]
/// - infection = [1,kI]
/// Close contacts (3rd elem):
/// - determined by kT (or 0 if quarantined)
/// </remarks>
using Host = std::tuple<short, short, short>;

/// <summary>
/// Representation for communicable diseases, suitable for  SEIRD model.
/// </summary>
/// <remarks>
/// <para>
/// Assumes exponential distributions for incubation time and duration of
/// infection, as well as a Poisson distribution for contacts per day. The
/// discrete geometric distribution is used in place of exponential due to
/// the nature of this discrete, stochastic simulation.
/// </para>
/// <para>
/// Default pathogen is modeled after Ebola. Our estimate for parameter
/// <c>pE</c>, probability of transmission per contact per day, is based on a
/// binomial distribution with mean of [1.4, 1.7] successes in 148.68 trials.
/// The range for the distribution mean is from Chowell and Nishiura (2015).
/// The number of trials is based on 9 days average infectious period and an
/// average daily contacts of 16.52, which is from Del Valle, et al. (2007).
/// </para>
/// <para>References:</para>
/// <list type="number">
///     <item><description>https://news.asu.edu/content/ebola-paper-demonstrates-disease-transmission-rate</description></item>
///     <item><description>https://pubmed.ncbi.nlm.nih.gov/25607595/</description></item>
///     <item><description>https://www.researchgate.net/publication/228649013_Mixing_patterns_between_age_groups_in_social_networks</description></item>
/// </list>
/// </remarks>
/// \todo Ask Glomski about how to compute analytically the probability of
/// transmission on a single contact given (a) the average number of contacts
/// infected by an infected individual, (b) the distribution of number of days
/// infectious.
class Pathogen
{
public:
    /// <summary>
    /// Initialize this disease.
    /// </summary>
    /// <param name="name">disease name</param>
    /// <param name="pE">probability of transmission per contact per day</param>
    /// <param name="pD">probability of death given infection</param>
    /// <param name="minE">minimum days after exposure until symptoms present</param>
    /// <param name="kE">average asymptomatic incubation time</param>
    /// <param name="minI">minimum days duration of infection</param>
    /// <param name="kI">average duration of infection</param>
    /// <param name="kT">average number of contacts per day</param>
    /// <param name="kQ">days duration of quarantine</param>
    Pathogen(std::string name = "Ebola", double pE = 0.005, double pD = 0.5,
        short minE = 2, short kE = 9, short minI = 7, short kI = 9,
        short kT = 16, short kQ = 1)
        : name(name), pcatch(pE), pdie(pD), edist(1.0f / (kE - minE + 1)),
        idist(1.0f / (kI - minI + 1)), ndist(kT), minE(minE), minI(minI),
        timeQ(kQ)
    {}

    /// <summary>
    /// Indicates that an individual may contract the pathogen if exposed.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <returns><c>true</c> if the host has not yet been exposed, <c>false</c> otherwise</returns>
    bool isSusceptible(Host const& h) const { return std::get<0>(h) == 0; }

    /// <summary>
    /// Indicates that an individual has been exposed and may be incubating the pathogen.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <returns><c>true</c> if the host has recently been exposed, <c>false</c> otherwise</returns>
    bool isExposed(Host const& h) const { return std::get<0>(h) == 1; }

    /// <summary>
    /// Indicates whether an individual may spread the pathogen.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <returns><c>true</c> if the host can spread the pathogen, <c>false</c> otherwise</returns>
    bool isInfectious(Host const& h) const { return std::get<0>(h) == 2; }

    /// <summary>
    /// Indicates that an infection has run its course for a given individual.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <returns><c>true</c> if the infection has finished in the host, <c>false</c> otherwise</returns>
    bool hasRunCourse(Host const& h) const { return std::get<0>(h) == 3; }

    /// <summary>
    /// Indicates that an individual has recovered with immunity.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <returns><c>true</c> if the host survived the infection, <c>false</c> otherwise</returns>
    bool isRecovered(Host const& h) const { return std::get<0>(h) == 4; }

    /// <summary>
    /// Indicates that an individual succumbed to the infection.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <returns><c>true</c> if the host died as a result of the infection, <c>false</c> otherwise</returns>
    bool isDeceased(Host const& h) const { return std::get<0>(h) == 5; }

    /// <summary>
    /// Indicates whether an individual is presenting symptoms.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <returns><c>true</c> if the host is presenting symptoms, <c>false</c> otherwise</returns>
    bool isDetected(Host const& h) const
    {
        return isInfectious(h) && std::get<1>(h) < minI;
    }

    /// <summary>
    /// Possibly infect a susceptible host.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <remarks>
    /// The probability of infecting the host depends on the properties of the pathogen.
    /// </remarks>
    void expose(Host& h) const
    {
        if (will_catch()) infect(h);
    }

    /// <summary>
    /// Infect a host individual with this pathogen.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <remarks>
    /// Infection begins with an (possibly zero) incubation period.
    /// </remarks>
    void infect(Host& h) const
    {
        std::get<0>(h) = 1;
        std::get<1>(h) = incubationPeriod();
    }

    /// <summary>
    /// Resolve an infection in a host.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <remarks>
    /// The probability that an infection kills the host depends on properties of the pathogen.
    /// </remarks>
    void expire(Host& h) const
    {
        if (will_die()) {
            kill(h);
        }
        else {
            recover(h);
        }
    }

    /// <summary>
    /// Advance the infection by one day.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <remarks>
    /// The duration of infection is determine by properties of the pathogen.
    /// </remarks>
    void worsen(Host& h) const
    {
        if (--std::get<1>(h) == 0) {
            ++std::get<0>(h);
            if (hasRunCourse(h)) {
                expire(h);
            }
            else {
                std::get<1>(h) = infectionPeriod();
            }
        }
    }

    /// <summary>
    /// Survive the infection.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    /// <remarks>
    /// A recovered individual is no longer susceptible.
    /// </remarks>
    void recover(Host& h) const { std::get<0>(h) = 4; }

    /// <summary>
    /// Succumb the host to the infection.
    /// </summary>
    /// <param name="h">a potential host in the population</param>
    void kill(Host& h) const { std::get<0>(h) = 5; }

    /// <summary>
    /// Probabilistically determine whether an individual will contract an infection.
    /// </summary>
    /// <returns><c>true</c> if the infection will take hold, <c>false</c> otherwise</returns>
    bool will_catch() const { return pcatch(rng); }

    /// <summary>
    /// Probabilistically determine whether an individual will die from infection.
    /// </summary>
    /// <returns><c>true</c> if the infection will kill the host, <c>false</c> otherwise</returns>
    bool will_die() const { return pdie(rng); }

    /// <summary>
    /// Probabilistically determine the duration of incubation.
    /// </summary>
    /// <returns>number of days for a host to advance from exposed to infected</returns>
    /// <remarks>
    /// Incubation time tends to follow an exponential distribution,
    /// so here we use the discrete analog, geometric distribution,
    /// for stochastic simulation with discrete time steps.
    /// </remarks>
    short incubationPeriod() const { return minE + edist(rng); }

    /// <summary>
    /// Probabilistically determine the duration of infection.
    /// </summary>
    /// <returns>number of days for an infection to progress to resolution</returns>
    /// <remarks>
    /// Infection time tends to follow an exponential distribution,
    /// so here we use the discrete analog, geometric distribution,
    /// for stochastic simulation with discrete time steps.
    /// </remarks>
    short infectionPeriod() const { return minI + idist(rng); }

    /// <summary>
    /// Probabilistically determine the size of the contact neighborhood for a host.
    /// </summary>
    /// <returns>distance away from this individual at which the infection can still be passed</returns>
    /// <remarks>
    /// Here we also use a geometric distribution to model the number of close
    /// contacts and individual might have in our stochastic simulation.
    /// </remarks>
    short numNeighbors() const { return 1 + ndist(rng); }

private:
    std::string name;
    static std::default_random_engine rng;
    mutable std::bernoulli_distribution pcatch;
    mutable std::bernoulli_distribution pdie;
    mutable std::geometric_distribution<short> edist;
    mutable std::geometric_distribution<short> idist;
    mutable std::poisson_distribution<short> ndist;
    short minE;
    short minI;
    short timeQ;
};

std::default_random_engine Pathogen::rng{ std::random_device()() };

#endif /*HPP_PATHOGEN*/
