#ifndef MODEL_HPP
#define MODEL_HPP

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();
    virtual void LedToggleRequested(bool value);
    virtual void RegisterUpDown(int value);


protected:
    ModelListener* modelListener;
    bool valuetoggle;
    int counter;
    unsigned int reg;
};

#endif // MODEL_HPP
