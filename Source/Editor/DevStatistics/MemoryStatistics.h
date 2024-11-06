#pragma once
#include "Widget.h"


class JGMemoryStatisticsCategory;
class JGMemoryStatisticsContent;

JGCLASS()
class JGMemoryStatistcs : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

	JGPROPERTY()
	PSharedPtr<JGMemoryStatisticsCategory> Category;

	JGPROPERTY()
	PSharedPtr<JGMemoryStatisticsContent> Content;

public:
	virtual void OnInitialize() override;
	virtual void OnGenerateGUI() override;


	virtual PString  GetTitleName() const override;
	virtual const  HGuid& GetGUID() const override;
};





