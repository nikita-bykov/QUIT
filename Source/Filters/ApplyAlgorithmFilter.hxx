#ifndef APPLYALGORITHMFILTER_HXX
#define APPLYALGORITHMFILTER_HXX

#include "itkObjectFactory.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"

namespace itk {

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::ApplyAlgorithmFilter() {
	//std::cout <<  __PRETTY_FUNCTION__ << endl;
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
void ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::SetAlgorithm(const shared_ptr<TAlgorithm> &a) {
	//std::cout <<  __PRETTY_FUNCTION__ << endl;
	m_algorithm = a;
	// Inputs go: Data 0, Data 1, ..., Mask, Const 0, Const 1, ...
    // Only the data inputs are required, the others are optional
    this->SetNumberOfRequiredInputs(a->numInputs());
    // Outputs go: Residuals, Iterations, Parameter 0, Parameter 1, ...
    size_t totalOutputs = StartOutputs + m_algorithm->numOutputs();
	this->SetNumberOfRequiredOutputs(totalOutputs);
	for (size_t i = 0; i < totalOutputs; i++) {
		this->SetNthOutput(i, this->MakeOutput(i));
	}
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
shared_ptr<const TAlgorithm> ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetAlgorithm() const { return m_algorithm; }

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
void ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::SetScaleToMean(const bool s) { m_scale_to_mean = s; }

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
bool ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetScaleToMean() const { return m_scale_to_mean; }

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
void ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::SetPoolsize(const size_t n) { m_poolsize = n; }

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
RealTimeClock::TimeStampType ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetMeanEvalTime() const { return m_meanTime; }

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
SizeValueType ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetEvaluations() const { return m_evaluations; }

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
void ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::SetInput(const size_t i, const TDataVectorImage *image) {
	//std::cout <<  __PRETTY_FUNCTION__ << endl;
	if (i < m_algorithm->numInputs()) {
		this->SetNthInput(i, const_cast<TDataVectorImage*>(image));
	} else {
		throw(runtime_error("Data input exceeds range"));
	}
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
void ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::SetConst(const size_t i, const TScalarImage *image) {
	//std::cout <<  __PRETTY_FUNCTION__ << endl;
	if (i < m_algorithm->numConsts()) {
		this->SetNthInput(m_algorithm->numInputs() + 1 + i, const_cast<TScalarImage*>(image));
	} else {
		throw(runtime_error("ConstInput " + to_string(i) + " out of range (there are " + to_string(m_algorithm->numConsts()) + " inputs)"));
	}
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
void ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::SetMask(const TScalarImage *image) {
	//std::cout <<  __PRETTY_FUNCTION__ << endl;
	this->SetNthInput(m_algorithm->numInputs(), const_cast<TScalarImage*>(image));
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
auto ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetInput(const size_t i) const -> typename TDataVectorImage::ConstPointer {
	////std::cout <<  __PRETTY_FUNCTION__ << endl;
	if (i < m_algorithm->numInputs()) {
		return static_cast<const TDataVectorImage *> (this->ProcessObject::GetInput(i));
	} else {
		throw(runtime_error(__PRETTY_FUNCTION__ +
		                    string("Input ") + to_string(i) +
		                    " out of range (" + to_string(m_algorithm->numInputs()) + ")"));
	}
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
auto ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetConst(const size_t i) const -> typename TScalarImage::ConstPointer {
	////std::cout <<  __PRETTY_FUNCTION__ << endl;
	if (i < m_algorithm->numConsts()) {
		size_t index = m_algorithm->numInputs() + 1 + i;
		return static_cast<const TScalarImage *> (this->ProcessObject::GetInput(index));
	} else {
		throw(runtime_error("Get Data Input out of range."));
	}
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
auto ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetMask() const -> typename TScalarImage::ConstPointer {
	////std::cout <<  __PRETTY_FUNCTION__ << endl;
	return static_cast<const TScalarImage *>(this->ProcessObject::GetInput(m_algorithm->numInputs()));
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
DataObject::Pointer ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::MakeOutput(unsigned int idx) {
	//std::cout <<  __PRETTY_FUNCTION__ << endl;
	DataObject::Pointer output;
    if (idx == ResidualsOutput) {
		auto img = TScalarVectorImage::New();
		output = img;
    } else if (idx == IterationsOutput) {
        auto img = TIterationsImage::New();
        output = img;
    } else if (idx < (m_algorithm->numOutputs() + StartOutputs)) {
		output = (TScalarImage::New()).GetPointer();
	} else {
        itkExceptionMacro("Attempted to create output " << idx << ", this algorithm only has " << m_algorithm->numOutputs() << "+" << StartOutputs << " outputs.");
	}
	return output.GetPointer();
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
auto ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetOutput(const size_t i) -> TScalarImage *{
	////std::cout <<  __PRETTY_FUNCTION__ << endl;
	if (i < m_algorithm->numOutputs()) {
        return dynamic_cast<TScalarImage *>(this->ProcessObject::GetOutput(i+StartOutputs));
	} else {
        itkExceptionMacro("Requested output " << to_string(i) << " is past maximum (" << to_string(m_algorithm->numOutputs()) << ")");
	}
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
auto ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetResidOutput() -> TScalarVectorImage *{
    return dynamic_cast<TScalarVectorImage *>(this->ProcessObject::GetOutput(ResidualsOutput));
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
auto ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GetIterationsOutput() -> TIterationsImage *{
    return dynamic_cast<TIterationsImage *>(this->ProcessObject::GetOutput(IterationsOutput));
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
void ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GenerateOutputInformation() {
	//std::cout <<  __PRETTY_FUNCTION__ << endl;
	Superclass::GenerateOutputInformation();
	size_t size = 0;
	for (size_t i = 0; i < m_algorithm->numInputs(); i++) {
		size += this->GetInput(i)->GetNumberOfComponentsPerPixel();
    }
	if (m_algorithm->dataSize() != size) {
        throw(std::runtime_error(string(__PRETTY_FUNCTION__) + "Sequence size (" + to_string(m_algorithm->dataSize()) + ") does not match input size (" + to_string(size) + ")"));
	}
    if (size == 0) {
        throw(std::runtime_error(string(__PRETTY_FUNCTION__) + "Total input size cannot be 0"));
    }

    auto input =     this->GetInput(0);
    auto region =    input->GetLargestPossibleRegion();
    auto spacing =   input->GetSpacing();
    auto origin =    input->GetOrigin();
    auto direction = input->GetDirection();
    for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
        auto op = this->GetOutput(i);
        op->SetRegions(region);
        op->SetSpacing(spacing);
        op->SetOrigin(origin);
        op->SetDirection(direction);
        op->Allocate();
    }
    auto r = this->GetResidOutput();
    r->SetRegions(region);
    r->SetSpacing(spacing);
    r->SetOrigin(origin);
    r->SetDirection(direction);
    r->SetNumberOfComponentsPerPixel(size);
    r->Allocate();
    auto i = this->GetIterationsOutput();
    i->SetRegions(region);
    i->SetSpacing(spacing);
    i->SetOrigin(origin);
    i->SetDirection(direction);
    i->Allocate();
    //std::cout <<  "Finished " << __PRETTY_FUNCTION__ << endl;
}

template<typename TAlgorithm, typename TData, typename TScalar, unsigned int ImageDim>
//void ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::ThreadedGenerateData(const TRegion &region, ThreadIdType threadId) {
void ApplyAlgorithmFilter<TAlgorithm, TData, TScalar, ImageDim>::GenerateData() {
	//std::cout <<  __PRETTY_FUNCTION__ << std::endl;

    TRegion region = this->GetInput(0)->GetLargestPossibleRegion();
    //std::cout << region << std::endl;

	ProgressReporter progress(this, 0, region.GetNumberOfPixels(), 10);

	vector<ImageRegionConstIterator<TDataVectorImage>> dataIters(m_algorithm->numInputs());
	for (size_t i = 0; i < m_algorithm->numInputs(); i++) {
		dataIters[i] = ImageRegionConstIterator<TDataVectorImage>(this->GetInput(i), region);
	}

	ImageRegionConstIterator<TScalarImage> maskIter;
	const auto mask = this->GetMask();
	if (mask) {
		maskIter = ImageRegionConstIterator<TScalarImage>(mask, region);
	}
	vector<ImageRegionConstIterator<TScalarImage>> constIters(m_algorithm->numConsts());
	for (size_t i = 0; i < m_algorithm->numConsts(); i++) {
		typename TScalarImage::ConstPointer c = this->GetConst(i);
		if (c) {
			constIters[i] = ImageRegionConstIterator<TScalarImage>(c, region);
		}
	}
	vector<ImageRegionIterator<TScalarImage>> outputIters(m_algorithm->numOutputs());
	for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
		outputIters[i] = ImageRegionIterator<TScalarImage>(this->GetOutput(i), region);
	}
	ImageRegionIterator<TScalarVectorImage> residIter(this->GetResidOutput(), region);
    ImageRegionIterator<TIterationsImage> iterationsIter(this->GetIterationsOutput(), region);
	typedef typename TAlgorithm::TArray TArray;
    TimeProbe clock;
    QI::ThreadPool threadPool(m_poolsize);
	while(!dataIters[0].IsAtEnd()) {
		if (!mask || maskIter.Get()) {
            auto task = [=] {
                TArray outputs = TArray::Zero(m_algorithm->numOutputs());
                TArray resids =  TArray::Zero(m_algorithm->dataSize());
                typename TAlgorithm::TIterations iterations{0};
                
                TArray constants = m_algorithm->defaultConsts();
                for (size_t i = 0; i < constIters.size(); i++) {
                    if (this->GetConst(i)) {
                        constants[i] = constIters[i].Get();
                    }
                }
                typename TAlgorithm::TInput allData(m_algorithm->dataSize());
                size_t dataIndex = 0;
                for (size_t i = 0; i < m_algorithm->numInputs(); i++) {
                    VariableLengthVector<TData> dataVector = dataIters[i].Get();
                    Map<const Eigen::Array<TData, Eigen::Dynamic, 1>> data(dataVector.GetDataPointer(), dataVector.Size());
                    Eigen::Array<TData, Eigen::Dynamic, 1> scaled = data;
                    if (m_scale_to_mean) {
                        scaled /= scaled.mean();
                    }
                    allData.segment(dataIndex, data.rows()) = scaled.template cast<typename TAlgorithm::TScalar>();
                    dataIndex += data.rows();
                }
                m_algorithm->apply(allData, constants, outputs, resids, iterations);
                for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
                    outputIters[i].Set(static_cast<float>(outputs[i]));
                }
                ArrayXf residF = resids.template cast<float>();
                VariableLengthVector<float> residVector(residF.data(), m_algorithm->dataSize());
                residIter.Set(residVector);
                iterationsIter.Set(iterations);
            };
            threadPool.enqueue(task);
		} else {
            for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
                outputIters[i].Set(0);
            }
            VariableLengthVector<float> residZeros(m_algorithm->dataSize()); residZeros.Fill(0.);
            residIter.Set(residZeros);
            iterationsIter.Set(0);
        }
        
		if (this->GetMask())
			++maskIter;
		for (size_t i = 0; i < m_algorithm->numInputs(); i++) {
			++dataIters[i];
		}
		for (size_t i = 0; i < m_algorithm->numConsts(); i++) {
			if (this->GetConst(i))
				++constIters[i];
		}
		for (size_t i = 0; i < m_algorithm->numOutputs(); i++) {
			++outputIters[i];
		}
		++residIter;
        ++iterationsIter;
		progress.CompletedPixel();
	}
	//std::cout << "Finished " << __PRETTY_FUNCTION__ << std::endl;
}
} // namespace ITK

#endif // APPLYALGORITHMFILTER_HXX
