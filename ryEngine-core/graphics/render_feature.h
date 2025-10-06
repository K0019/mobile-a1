#pragma once

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

namespace internal
{
  class RenderPassBuilder;
}

// RT MEANS IT'S CALLED ON THE RENDER THREAD, DON'T FORGET THAT
class IRenderFeature
{
  public:
    virtual ~IRenderFeature() = default;

    //DEFINE THESE
    virtual const char* GetName() const = 0;

    virtual void SetupPasses(internal::RenderPassBuilder& passBuilder) = 0;

    // DO NOT DEFINE THESE
    // the pointer for the parameters for the game thread (STILL CALLED ON RENDER THREAD)
    virtual void* GetGTParameterBlock_RT() = 0;

    // the pointer for the parameters on the render thread
    virtual const void* GetParameterBlock_RT() = 0;

    //stores the buffer that the game thread wrote to for rendering,
    //and returns the new pointer to the game thread,
    //IT IS THE RENDER THREAD'S JOB TO CALL THIS. 
    virtual void* SwapBuffersForGT_RT() = 0;
};

template <typename Params, int BufferCount = MAX_FRAMES_IN_FLIGHT>
class RenderFeatureBase : public IRenderFeature 
{
  public:
    using ParameterType = Params;

  protected:
    ParameterType param_[BufferCount]{};
    int readBuffer_ = 0;

  public:
    void* GetGTParameterBlock_RT() override
    {
      return &param_[(readBuffer_ + 1) % BufferCount];
    }

    const void* GetParameterBlock_RT() override
    {
      return &param_[readBuffer_];
    }

    void* SwapBuffersForGT_RT() override
    {
      readBuffer_ = (readBuffer_ + 1) % BufferCount;
      return GetGTParameterBlock_RT();
    }
};
