package com.ewolff.microservice.order.logic;

import javax.persistence.Entity;
import javax.persistence.GeneratedValue;
import javax.persistence.Id;
import javax.persistence.Table;

import org.apache.commons.lang.builder.EqualsBuilder;
import org.apache.commons.lang.builder.HashCodeBuilder;
import org.apache.commons.lang.builder.ToStringBuilder;

@Entity
@Table(name = "SIMPLE_ORDER")
public class SimpleOrder {

	@Id
	@GeneratedValue
	private long orderId;

	private String customerName;
	
	private String itemOrdered;
	
	private int quantity;
	
	private double price;

	public SimpleOrder() {
		super();
	}

	public SimpleOrder(String customerName, String itemOrdered, int quantity, double price) {
		super();
		this.customerName = customerName;
		this.itemOrdered = itemOrdered;
		this.quantity = quantity;
		this.price = price;
	}

	public long getOrderId() {
		return orderId;
	}

	public void setOrderId(long orderId) {
		this.orderId = orderId;
	}

	public String getCustomerName() {
		return customerName;
	}

	public void setCustomerName(String customerName) {
		this.customerName = customerName;
	}

	public String getItemOrdered() {
		return itemOrdered;
	}

	public void setItemOrdered(String itemOrdered) {
		this.itemOrdered = itemOrdered;
	}

	public int getQuantity() {
		return quantity;
	}

	public void setQuantity(int quantity) {
		this.quantity = quantity;
	}

	public double getPrice() {
		return price;
	}

	public void setPrice(double price) {
		this.price = price;
	}

	@Override
	public String toString() {
		return ToStringBuilder.reflectionToString(this);
	}

	@Override
	public int hashCode() {
		return HashCodeBuilder.reflectionHashCode(this);
	}

	@Override
	public boolean equals(Object obj) {
		return EqualsBuilder.reflectionEquals(this, obj);
	}
}
