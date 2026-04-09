#pragma once

#include "CoreMinimal.h"
#include "ShipModuleTypes.generated.h"

/**
 * Функциональный тип модуля корабля.
 * Определяет назначение модуля в конструкторе и влияет на доступные роли,
 * типы миссий и характеристики корабля.
 */
UENUM(BlueprintType)
enum class EShipModuleType : uint8
{
	/** Капитанский мостик: командный центр с местами капитана и пилота. */
	Bridge,
	/** Грузовой отсек: хранение грузов, допуск к контрактам на доставку. */
	CargoHold,
	/** Научная лаборатория: исследовательские миссии и сканирование. */
	ScienceLab,
	/** Пассажирская каюта: перевозка пассажиров. */
	PassengerCabin,
	/** Турель: точка вооружения, требует стрелка. */
	Turret,
	/** Топливный бак: запас топлива для полёта. */
	FuelTank,
	/** Кислородный бак: запас кислорода для экипажа. */
	OxygenTank,
	/** Двигатель: обеспечивает тягу и маневренность. */
	Engine,
	/** Генератор щитов: защита корабля от урона. */
	ShieldGenerator,
	/** Реактор: источник энергии для всех систем. */
	Reactor,
	/** Сенсорный массив: обнаружение угроз и навигация. */
	Sensor,
	/** Модуль связи: коммуникации и координация. */
	Communication,
	/** Пост охраны: внутренняя безопасность и абордаж. */
	SecurityPost,
	/** Коридор: соединительный проход между модулями. */
	Corridor,
	/** Посадочный шлюз: вход/выход экипажа из корабля. Обязателен для валидной сборки. */
	Airlock
};

/**
 * Тип стыковочного соединения контактной точки модуля.
 * Определяет, какой проход появится при стыковке двух модулей.
 */
UENUM(BlueprintType)
enum class EShipModuleSocketType : uint8
{
	/** Горизонтальный дверной проём (стандартный проход между модулями). */
	Horizontal,
	/** Вертикальный люк (переход между палубами вверх/вниз). */
	Vertical,
	/** Универсальный: совместим с любой ориентацией. */
	Universal
};

/**
 * Контактная точка (стыковочный узел) модуля корабля.
 * Определяет место и тип возможного соединения с другим модулем.
 */
USTRUCT(BlueprintType)
struct FShipModuleContactPoint
{
	GENERATED_BODY()

	/** Уникальное имя точки в пределах модуля (например, "Front", "Left", "Top"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ContactPoint")
	FName SocketName;

	/** Позиция относительно начала координат модуля (в см). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ContactPoint")
	FVector RelativeLocation = FVector::ZeroVector;

	/** Ориентация контактной точки. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ContactPoint")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	/** Тип соединения: горизонтальный проход, вертикальный люк или универсальный. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ContactPoint")
	EShipModuleSocketType SocketType = EShipModuleSocketType::Horizontal;
};
