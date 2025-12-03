/*********************************************************************
 * @file  dispenser.hpp
 * @author Burgmeier Timothée & Louis-le-Denmat Raphaël
 * @brief Dispenser header file
 *********************************************************************/
#ifndef DISPENSER_HPP
#define DISPENSER_HPP

/**
 * @class food_dispenser
 * @brief food_dispenser Class
 */
class food_dispenser
{
public:
    /**
     * @fn food_dispenser();
     * @brief Default constructor
     */
    food_dispenser();
    /**
     * @fn food_dispenser();
     * @brief Destructor
     */
    ~food_dispenser();
    /**
     * @fn void init(void)
     * @brief Initialization function of the food dispenser
     */
    void init(void);
    /**
     * @fn void run(void)
     * @brief Run function of the food dispenser
     */
    void run(void);
};

#endif // DISPENSER_HPP